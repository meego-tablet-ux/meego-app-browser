/*
 Copyright (C) Intel Corp.  2006.  All Rights Reserved.
 Intel funded Tungsten Graphics (http://www.tungstengraphics.com) to
 develop this 3D driver.
 
 Permission is hereby granted, free of charge, to any person obtaining
 a copy of this software and associated documentation files (the
 "Software"), to deal in the Software without restriction, including
 without limitation the rights to use, copy, modify, merge, publish,
 distribute, sublicense, and/or sell copies of the Software, and to
 permit persons to whom the Software is furnished to do so, subject to
 the following conditions:
 
 The above copyright notice and this permission notice (including the
 next paragraph) shall be included in all copies or substantial
 portions of the Software.
 
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
 LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 
 **********************************************************************/
 /*
  * Authors:
  *   Keith Whitwell <keith@tungstengraphics.com>
  */

/** @file brw_state_cache.c
 *
 * This file implements a simple static state cache for 965.  The consumers
 * can query the hash table of state using a cache_id, opaque key data,
 * and list of buffers that will be used in relocations, and receive the
 * corresponding state buffer object of state (plus associated auxiliary
 * data) in return.
 *
 * The inner workings are a simple hash table based on a CRC of the key data.
 * The cache_id and relocation target buffers associated with the state
 * buffer are included as auxiliary key data, but are not part of the hash
 * value (this should be fixed, but will likely be fixed instead by making
 * consumers use structured keys).
 *
 * Replacement is not implemented.  Instead, when the cache gets too big, at
 * a safe point (unlock) we throw out all of the cache data and let it
 * regenerate for the next rendering operation.
 *
 * The reloc_buf pointers need to be included as key data, otherwise the
 * non-unique values stuffed in the offset in key data through
 * brw_cache_data() may result in successful probe for state buffers
 * even when the buffer being referenced doesn't match.  The result would be
 * that the same state cache entry is used twice for different buffers,
 * only one of the two buffers referenced gets put into the offset, and the
 * incorrect program is run for the other instance.
 */

#include "main/imports.h"
#include "brw_state.h"
#include "intel_batchbuffer.h"

/* XXX: Fixme - have to include these to get the sizes of the prog_key
 * structs:
 */
#include "brw_wm.h"
#include "brw_vs.h"
#include "brw_clip.h"
#include "brw_sf.h"
#include "brw_gs.h"


static GLuint
hash_key(const void *key, GLuint key_size,
         dri_bo **reloc_bufs, GLuint nr_reloc_bufs)
{
   GLuint *ikey = (GLuint *)key;
   GLuint hash = 0, i;

   assert(key_size % 4 == 0);

   /* I'm sure this can be improved on:
    */
   for (i = 0; i < key_size/4; i++) {
      hash ^= ikey[i];
      hash = (hash << 5) | (hash >> 27);
   }

   /* Include the BO pointers as key data as well */
   ikey = (GLuint *)reloc_bufs;
   key_size = nr_reloc_bufs * sizeof(dri_bo *);
   for (i = 0; i < key_size/4; i++) {
      hash ^= ikey[i];
      hash = (hash << 5) | (hash >> 27);
   }

   return hash;
}


/**
 * Marks a new buffer as being chosen for the given cache id.
 */
static void
update_cache_last(struct brw_cache *cache, enum brw_cache_id cache_id,
		  dri_bo *bo)
{
   if (bo == cache->last_bo[cache_id])
      return; /* no change */

   dri_bo_unreference(cache->last_bo[cache_id]);
   cache->last_bo[cache_id] = bo;
   dri_bo_reference(cache->last_bo[cache_id]);
   cache->brw->state.dirty.cache |= 1 << cache_id;
}


static struct brw_cache_item *
search_cache(struct brw_cache *cache, enum brw_cache_id cache_id,
	     GLuint hash, const void *key, GLuint key_size,
	     dri_bo **reloc_bufs, GLuint nr_reloc_bufs)
{
   struct brw_cache_item *c;

#if 0
   int bucketcount = 0;

   for (c = cache->items[hash % cache->size]; c; c = c->next)
      bucketcount++;

   fprintf(stderr, "bucket %d/%d = %d/%d items\n", hash % cache->size,
	   cache->size, bucketcount, cache->n_items);
#endif

   for (c = cache->items[hash % cache->size]; c; c = c->next) {
      if (c->cache_id == cache_id &&
	  c->hash == hash &&
	  c->key_size == key_size &&
	  memcmp(c->key, key, key_size) == 0 &&
	  c->nr_reloc_bufs == nr_reloc_bufs &&
	  memcmp(c->reloc_bufs, reloc_bufs,
		 nr_reloc_bufs * sizeof(dri_bo *)) == 0)
	 return c;
   }

   return NULL;
}


static void
rehash(struct brw_cache *cache)
{
   struct brw_cache_item **items;
   struct brw_cache_item *c, *next;
   GLuint size, i;

   size = cache->size * 3;
   items = (struct brw_cache_item**) _mesa_calloc(size * sizeof(*items));

   for (i = 0; i < cache->size; i++)
      for (c = cache->items[i]; c; c = next) {
	 next = c->next;
	 c->next = items[c->hash % size];
	 items[c->hash % size] = c;
      }

   FREE(cache->items);
   cache->items = items;
   cache->size = size;
}


/**
 * Returns the buffer object matching cache_id and key, or NULL.
 */
dri_bo *
brw_search_cache(struct brw_cache *cache,
                 enum brw_cache_id cache_id,
                 const void *key,
                 GLuint key_size,
                 dri_bo **reloc_bufs, GLuint nr_reloc_bufs,
                 void *aux_return)
{
   struct brw_cache_item *item;
   GLuint hash = hash_key(key, key_size, reloc_bufs, nr_reloc_bufs);

   item = search_cache(cache, cache_id, hash, key, key_size,
		       reloc_bufs, nr_reloc_bufs);

   if (item == NULL)
      return NULL;

   if (aux_return)
      *(void **)aux_return = (void *)((char *)item->key + item->key_size);

   update_cache_last(cache, cache_id, item->bo);

   dri_bo_reference(item->bo);
   return item->bo;
}


dri_bo *
brw_upload_cache( struct brw_cache *cache,
		  enum brw_cache_id cache_id,
		  const void *key,
		  GLuint key_size,
		  dri_bo **reloc_bufs,
		  GLuint nr_reloc_bufs,
		  const void *data,
		  GLuint data_size,
		  const void *aux,
		  void *aux_return )
{
   struct brw_cache_item *item = CALLOC_STRUCT(brw_cache_item);
   GLuint hash = hash_key(key, key_size, reloc_bufs, nr_reloc_bufs);
   GLuint relocs_size = nr_reloc_bufs * sizeof(dri_bo *);
   GLuint aux_size = cache->aux_size[cache_id];
   void *tmp;
   dri_bo *bo;
   int i;

   /* Create the buffer object to contain the data */
   bo = dri_bo_alloc(cache->brw->intel.bufmgr,
		     cache->name[cache_id], data_size, 1 << 6);


   /* Set up the memory containing the key, aux_data, and reloc_bufs */
   tmp = _mesa_malloc(key_size + aux_size + relocs_size);

   memcpy(tmp, key, key_size);
   memcpy(tmp + key_size, aux, cache->aux_size[cache_id]);
   memcpy(tmp + key_size + aux_size, reloc_bufs, relocs_size);
   for (i = 0; i < nr_reloc_bufs; i++) {
      if (reloc_bufs[i] != NULL)
	 dri_bo_reference(reloc_bufs[i]);
   }

   item->cache_id = cache_id;
   item->key = tmp;
   item->hash = hash;
   item->key_size = key_size;
   item->reloc_bufs = tmp + key_size + aux_size;
   item->nr_reloc_bufs = nr_reloc_bufs;

   item->bo = bo;
   dri_bo_reference(bo);

   if (cache->n_items > cache->size * 1.5)
      rehash(cache);

   hash %= cache->size;
   item->next = cache->items[hash];
   cache->items[hash] = item;
   cache->n_items++;

   if (aux_return) {
      assert(cache->aux_size[cache_id]);
      *(void **)aux_return = (void *)((char *)item->key + item->key_size);
   }

   if (INTEL_DEBUG & DEBUG_STATE)
      _mesa_printf("upload %s: %d bytes to cache id %d\n",
		   cache->name[cache_id],
		   data_size, cache_id);

   /* Copy data to the buffer */
   dri_bo_subdata(bo, 0, data_size, data);

   update_cache_last(cache, cache_id, bo);

   return bo;
}


/**
 * Wrapper around brw_cache_data_sz using the cache_id's canonical key size.
 *
 * If nr_reloc_bufs is nonzero, brw_search_cache()/brw_upload_cache() would be
 * better to use, as the potentially changing offsets in the data-used-as-key
 * will result in excessive cache misses.
 *
 * If aux data is involved, use search/upload instead.

 */
dri_bo *
brw_cache_data(struct brw_cache *cache,
	       enum brw_cache_id cache_id,
	       const void *data,
	       GLuint data_size,
	       dri_bo **reloc_bufs,
	       GLuint nr_reloc_bufs)
{
   dri_bo *bo;
   struct brw_cache_item *item;
   GLuint hash = hash_key(data, data_size, reloc_bufs, nr_reloc_bufs);

   item = search_cache(cache, cache_id, hash, data, data_size,
		       reloc_bufs, nr_reloc_bufs);
   if (item) {
      update_cache_last(cache, cache_id, item->bo);
      dri_bo_reference(item->bo);
      return item->bo;
   }

   bo = brw_upload_cache(cache, cache_id,
			 data, data_size,
			 reloc_bufs, nr_reloc_bufs,
			 data, data_size,
			 NULL, NULL);

   return bo;
}

enum pool_type {
   DW_SURFACE_STATE,
   DW_GENERAL_STATE
};


static void
brw_init_cache_id(struct brw_cache *cache,
                  const char *name,
                  enum brw_cache_id id,
                  GLuint aux_size)
{
   cache->name[id] = strdup(name);
   cache->aux_size[id] = aux_size;
}


static void
brw_init_non_surface_cache(struct brw_context *brw)
{
   struct brw_cache *cache = &brw->cache;

   cache->brw = brw;

   cache->size = 7;
   cache->n_items = 0;
   cache->items = (struct brw_cache_item **)
      _mesa_calloc(cache->size * sizeof(struct brw_cache_item));

   brw_init_cache_id(cache,
		     "CC_VP",
		     BRW_CC_VP,
		     0);

   brw_init_cache_id(cache,
		     "CC_UNIT",
		     BRW_CC_UNIT,
		     0);

   brw_init_cache_id(cache,
		     "WM_PROG",
		     BRW_WM_PROG,
		     sizeof(struct brw_wm_prog_data));

   brw_init_cache_id(cache,
		     "SAMPLER_DEFAULT_COLOR",
		     BRW_SAMPLER_DEFAULT_COLOR,
		     0);

   brw_init_cache_id(cache,
		     "SAMPLER",
		     BRW_SAMPLER,
		     0);

   brw_init_cache_id(cache,
		     "WM_UNIT",
		     BRW_WM_UNIT,
		     0);

   brw_init_cache_id(cache,
		     "SF_PROG",
		     BRW_SF_PROG,
		     sizeof(struct brw_sf_prog_data));

   brw_init_cache_id(cache,
		     "SF_VP",
		     BRW_SF_VP,
		     0);

   brw_init_cache_id(cache,
		     "SF_UNIT",
		     BRW_SF_UNIT,
		     0);

   brw_init_cache_id(cache,
		     "VS_UNIT",
		     BRW_VS_UNIT,
		     0);

   brw_init_cache_id(cache,
		     "VS_PROG",
		     BRW_VS_PROG,
		     sizeof(struct brw_vs_prog_data));

   brw_init_cache_id(cache,
		     "CLIP_UNIT",
		     BRW_CLIP_UNIT,
		     0);

   brw_init_cache_id(cache,
		     "CLIP_PROG",
		     BRW_CLIP_PROG,
		     sizeof(struct brw_clip_prog_data));

   brw_init_cache_id(cache,
		     "GS_UNIT",
		     BRW_GS_UNIT,
		     0);

   brw_init_cache_id(cache,
		     "GS_PROG",
		     BRW_GS_PROG,
		     sizeof(struct brw_gs_prog_data));
}


static void
brw_init_surface_cache(struct brw_context *brw)
{
   struct brw_cache *cache = &brw->surface_cache;

   cache->brw = brw;

   cache->size = 7;
   cache->n_items = 0;
   cache->items = (struct brw_cache_item **)
      _mesa_calloc(cache->size * sizeof(struct brw_cache_item));

   brw_init_cache_id(cache,
		     "SS_SURFACE",
		     BRW_SS_SURFACE,
		     0);

   brw_init_cache_id(cache,
		     "SS_SURF_BIND",
		     BRW_SS_SURF_BIND,
		     0);
}


void
brw_init_caches(struct brw_context *brw)
{
   brw_init_non_surface_cache(brw);
   brw_init_surface_cache(brw);
}


static void
brw_clear_cache(struct brw_context *brw, struct brw_cache *cache)
{
   struct brw_cache_item *c, *next;
   GLuint i;

   if (INTEL_DEBUG & DEBUG_STATE)
      _mesa_printf("%s\n", __FUNCTION__);

   for (i = 0; i < cache->size; i++) {
      for (c = cache->items[i]; c; c = next) {
	 int j;

	 next = c->next;
	 for (j = 0; j < c->nr_reloc_bufs; j++)
	    dri_bo_unreference(c->reloc_bufs[j]);
	 dri_bo_unreference(c->bo);
	 free((void *)c->key);
	 free(c);
      }
      cache->items[i] = NULL;
   }

   cache->n_items = 0;

   if (brw->curbe.last_buf) {
      _mesa_free(brw->curbe.last_buf);
      brw->curbe.last_buf = NULL;
   }

   brw->state.dirty.mesa |= ~0;
   brw->state.dirty.brw |= ~0;
   brw->state.dirty.cache |= ~0;
}

/* Clear all entries from the cache that point to the given bo.
 *
 * This lets us release memory for reuse earlier for known-dead buffers,
 * at the cost of walking the entire hash table.
 */
void
brw_state_cache_bo_delete(struct brw_cache *cache, dri_bo *bo)
{
   struct brw_cache_item **prev;
   GLuint i;

   if (INTEL_DEBUG & DEBUG_STATE)
      _mesa_printf("%s\n", __FUNCTION__);

   for (i = 0; i < cache->size; i++) {
      for (prev = &cache->items[i]; *prev;) {
	 struct brw_cache_item *c = *prev;

	 if (drm_intel_bo_references(c->bo, bo)) {
	    int j;

	    *prev = c->next;

	    for (j = 0; j < c->nr_reloc_bufs; j++)
	       dri_bo_unreference(c->reloc_bufs[j]);
	    dri_bo_unreference(c->bo);
	    free((void *)c->key);
	    free(c);
	    cache->n_items--;
	 } else {
	    prev = &c->next;
	 }
      }
   }
}

void
brw_state_cache_check_size(struct brw_context *brw)
{
   if (INTEL_DEBUG & DEBUG_STATE)
      _mesa_printf("%s (n_items=%d)\n", __FUNCTION__, brw->cache.n_items);

   /* un-tuned guess.  We've got around 20 state objects for a total of around
    * 32k, so 1000 of them is around 1.5MB.
    */
   if (brw->cache.n_items > 1000)
      brw_clear_cache(brw, &brw->cache);

   if (brw->surface_cache.n_items > 1000)
      brw_clear_cache(brw, &brw->surface_cache);
}


static void
brw_destroy_cache(struct brw_context *brw, struct brw_cache *cache)
{
   GLuint i;

   if (INTEL_DEBUG & DEBUG_STATE)
      _mesa_printf("%s\n", __FUNCTION__);

   brw_clear_cache(brw, cache);
   for (i = 0; i < BRW_MAX_CACHE; i++) {
      dri_bo_unreference(cache->last_bo[i]);
      free(cache->name[i]);
   }
   free(cache->items);
   cache->items = NULL;
   cache->size = 0;
}


void
brw_destroy_caches(struct brw_context *brw)
{
   brw_destroy_cache(brw, &brw->cache);
   brw_destroy_cache(brw, &brw->surface_cache);
}
