/*
 * copyright (c) 2001 Fabrice Bellard
 *
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef AVFORMAT_AVFORMAT_H
#define AVFORMAT_AVFORMAT_H

#define LIBAVFORMAT_VERSION_MAJOR 52
#define LIBAVFORMAT_VERSION_MINOR 23
#define LIBAVFORMAT_VERSION_MICRO  1

#define LIBAVFORMAT_VERSION_INT AV_VERSION_INT(LIBAVFORMAT_VERSION_MAJOR, \
                                               LIBAVFORMAT_VERSION_MINOR, \
                                               LIBAVFORMAT_VERSION_MICRO)
#define LIBAVFORMAT_VERSION     AV_VERSION(LIBAVFORMAT_VERSION_MAJOR,   \
                                           LIBAVFORMAT_VERSION_MINOR,   \
                                           LIBAVFORMAT_VERSION_MICRO)
#define LIBAVFORMAT_BUILD       LIBAVFORMAT_VERSION_INT

#define LIBAVFORMAT_IDENT       "Lavf" AV_STRINGIFY(LIBAVFORMAT_VERSION)

/**
 * Returns the LIBAVFORMAT_VERSION_INT constant.
 */
unsigned avformat_version(void);

#include <time.h>
#include <stdio.h>  /* FILE */
#include "libavcodec/avcodec.h"

#include "avio.h"


/*
 * Public Metadata API.
 * The metadata API allows libavformat to export metadata tags to a client
 * application using a sequence of key/value pairs.
 * Important concepts to keep in mind:
 * 1. Keys are unique; there can never be 2 tags with the same key. This is
 *    also meant semantically, i.e., a demuxer should not knowingly produce
 *    several keys that are literally different but semantically identical.
 *    E.g., key=Author5, key=Author6. In this example, all authors must be
 *    placed in the same tag.
 * 2. Metadata is flat, not hierarchical; there are no subtags. If you
 *    want to store, e.g., the email address of the child of producer Alice
 *    and actor Bob, that could have key=alice_and_bobs_childs_email_address.
 * 3. A tag whose value is localized for a particular language is appended
 *    with a dash character ('-') and the ISO 639 3-letter language code.
 *    For example: Author-ger=Michael, Author-eng=Mike
 *    The original/default language is in the unqualified "Author" tag.
 *    A demuxer should set a default if it sets any translated tag.
 */

#define AV_METADATA_IGNORE_CASE     1
#define AV_METADATA_IGNORE_SUFFIX   2

typedef struct {
    char *key;
    char *value;
}AVMetadataTag;

typedef struct AVMetadata AVMetadata;

/**
 * gets a metadata element with matching key.
 * @param prev set to the previous matching element to find the next.
 * @param flags allows case as well as suffix insensitive comparissions.
 * @return found tag or NULL, changing key or value leads to undefined behavior.
 */
AVMetadataTag *
av_metadata_get(AVMetadata *m, const char *key, const AVMetadataTag *prev, int flags);

/**
 * sets the given tag in m, overwriting an existing tag.
 * @param tag tag to add to m, key and value will be av_strduped.
 * @return >= 0 if success otherwise error code that is <0.
 */
int av_metadata_set(AVMetadata **m, AVMetadataTag tag);

/**
 * Free all the memory allocated for an AVMetadata struct.
 */
void av_metadata_free(AVMetadata **m);


/* packet functions */

typedef struct AVPacket {
    /**
     * Presentation timestamp in time_base units.
     * This is the time at which the decompressed packet will be presented
     * to the user.
     * Can be AV_NOPTS_VALUE if it is not stored in the file.
     * pts MUST be larger or equal to dts as presentation cannot happen before
     * decompression, unless one wants to view hex dumps. Some formats misuse
     * the terms dts and pts/cts to mean something different, these timestamps
     * must be converted to true pts/dts before they are stored in AVPacket.
     */
    int64_t pts;
    /**
     * Decompression timestamp in time_base units.
     * This is the time at which the packet is decompressed.
     * Can be AV_NOPTS_VALUE if it is not stored in the file.
     */
    int64_t dts;
    uint8_t *data;
    int   size;
    int   stream_index;
    int   flags;
    /**
     * Duration of this packet in time_base units, 0 if unknown.
     * Equals next_pts - this_pts in presentation order.
     */
    int   duration;
    void  (*destruct)(struct AVPacket *);
    void  *priv;
    int64_t pos;                            ///< byte position in stream, -1 if unknown

    /**
     * Time difference in stream time base units from the pts of this
     * packet to the point at which the output from the decoder has converged
     * independent from the availability of previous frames. That is, the
     * frames are virtually identical no matter if decoding started from
     * the very first frame or from this keyframe.
     * Is AV_NOPTS_VALUE if unknown.
     * This field is not the display duration of the current packet.
     *
     * The purpose of this field is to allow seeking in streams that have no
     * keyframes in the conventional sense. It corresponds to the
     * recovery point SEI in H.264 and match_time_delta in NUT. It is also
     * essential for some types of subtitle streams to ensure that all
     * subtitles are correctly displayed after seeking.
     */
    int64_t convergence_duration;
} AVPacket;
#define PKT_FLAG_KEY   0x0001

void av_destruct_packet_nofree(AVPacket *pkt);

/**
 * Default packet destructor.
 */
void av_destruct_packet(AVPacket *pkt);

/**
 * Initialize optional fields of a packet with default values.
 *
 * @param pkt packet
 */
void av_init_packet(AVPacket *pkt);

/**
 * Allocate the payload of a packet and initialize its fields with
 * default values.
 *
 * @param pkt packet
 * @param size wanted payload size
 * @return 0 if OK, AVERROR_xxx otherwise
 */
int av_new_packet(AVPacket *pkt, int size);

/**
 * Allocate and read the payload of a packet and initialize its fields with
 * default values.
 *
 * @param pkt packet
 * @param size desired payload size
 * @return >0 (read size) if OK, AVERROR_xxx otherwise
 */
int av_get_packet(ByteIOContext *s, AVPacket *pkt, int size);

/**
 * @warning This is a hack - the packet memory allocation stuff is broken. The
 * packet is allocated if it was not really allocated.
 */
int av_dup_packet(AVPacket *pkt);

/**
 * Free a packet.
 *
 * @param pkt packet to free
 */
static inline void av_free_packet(AVPacket *pkt)
{
    if (pkt && pkt->destruct) {
        pkt->destruct(pkt);
    }
}

/*************************************************/
/* fractional numbers for exact pts handling */

/**
 * The exact value of the fractional number is: 'val + num / den'.
 * num is assumed to be 0 <= num < den.
 * @deprecated Use AVRational instead.
*/
typedef struct AVFrac {
    int64_t val, num, den;
} AVFrac;

/*************************************************/
/* input/output formats */

struct AVCodecTag;

struct AVFormatContext;

/** This structure contains the data a format has to probe a file. */
typedef struct AVProbeData {
    const char *filename;
    unsigned char *buf;
    int buf_size;
} AVProbeData;

#define AVPROBE_SCORE_MAX 100               ///< Maximum score, half of that is used for file-extension-based detection.
#define AVPROBE_PADDING_SIZE 32             ///< extra allocated bytes at the end of the probe buffer

typedef struct AVFormatParameters {
    AVRational time_base;
    int sample_rate;
    int channels;
    int width;
    int height;
    enum PixelFormat pix_fmt;
    int channel; /**< Used to select DV channel. */
    const char *standard; /**< TV standard, NTSC, PAL, SECAM */
    unsigned int mpeg2ts_raw:1;  /**< Force raw MPEG-2 transport stream output, if possible. */
    unsigned int mpeg2ts_compute_pcr:1; /**< Compute exact PCR for each transport
                                            stream packet (only meaningful if
                                            mpeg2ts_raw is TRUE). */
    unsigned int initial_pause:1;       /**< Do not begin to play the stream
                                            immediately (RTSP only). */
    unsigned int prealloced_context:1;
#if LIBAVFORMAT_VERSION_INT < (53<<16)
    enum CodecID video_codec_id;
    enum CodecID audio_codec_id;
#endif
} AVFormatParameters;

//! Demuxer will use url_fopen, no opened file should be provided by the caller.
#define AVFMT_NOFILE        0x0001
#define AVFMT_NEEDNUMBER    0x0002 /**< Needs '%d' in filename. */
#define AVFMT_SHOW_IDS      0x0008 /**< Show format stream IDs numbers. */
#define AVFMT_RAWPICTURE    0x0020 /**< Format wants AVPicture structure for
                                      raw picture data. */
#define AVFMT_GLOBALHEADER  0x0040 /**< Format wants global header. */
#define AVFMT_NOTIMESTAMPS  0x0080 /**< Format does not need / have any timestamps. */
#define AVFMT_GENERIC_INDEX 0x0100 /**< Use generic index building code. */
#define AVFMT_TS_DISCONT    0x0200 /**< Format allows timestamp discontinuities. */

typedef struct AVOutputFormat {
    const char *name;
    /**
     * Descriptive name for the format, meant to be more human-readable
     * than \p name. You \e should use the NULL_IF_CONFIG_SMALL() macro
     * to define it.
     */
    const char *long_name;
    const char *mime_type;
    const char *extensions; /**< comma-separated filename extensions */
    /** Size of private data so that it can be allocated in the wrapper. */
    int priv_data_size;
    /* output support */
    enum CodecID audio_codec; /**< default audio codec */
    enum CodecID video_codec; /**< default video codec */
    int (*write_header)(struct AVFormatContext *);
    int (*write_packet)(struct AVFormatContext *, AVPacket *pkt);
    int (*write_trailer)(struct AVFormatContext *);
    /** can use flags: AVFMT_NOFILE, AVFMT_NEEDNUMBER, AVFMT_GLOBALHEADER */
    int flags;
    /** Currently only used to set pixel format if not YUV420P. */
    int (*set_parameters)(struct AVFormatContext *, AVFormatParameters *);
    int (*interleave_packet)(struct AVFormatContext *, AVPacket *out,
                             AVPacket *in, int flush);

    /**
     * List of supported codec_id-codec_tag pairs, ordered by "better
     * choice first". The arrays are all CODEC_ID_NONE terminated.
     */
    const struct AVCodecTag * const *codec_tag;

    enum CodecID subtitle_codec; /**< default subtitle codec */

    /* private fields */
    struct AVOutputFormat *next;
} AVOutputFormat;

typedef struct AVInputFormat {
    const char *name;
    /**
     * Descriptive name for the format, meant to be more human-readable
     * than \p name. You \e should use the NULL_IF_CONFIG_SMALL() macro
     * to define it.
     */
    const char *long_name;
    /** Size of private data so that it can be allocated in the wrapper. */
    int priv_data_size;
    /**
     * Tell if a given file has a chance of being parsed by this format.
     * The buffer provided is guaranteed to be AVPROBE_PADDING_SIZE bytes
     * big so you do not have to check for that unless you need more.
     */
    int (*read_probe)(AVProbeData *);
    /** Read the format header and initialize the AVFormatContext
       structure. Return 0 if OK. 'ap' if non-NULL contains
       additional parameters. Only used in raw format right
       now. 'av_new_stream' should be called to create new streams.  */
    int (*read_header)(struct AVFormatContext *,
                       AVFormatParameters *ap);
    /** Read one packet and put it in 'pkt'. pts and flags are also
       set. 'av_new_stream' can be called only if the flag
       AVFMTCTX_NOHEADER is used. */
    int (*read_packet)(struct AVFormatContext *, AVPacket *pkt);
    /** Close the stream. The AVFormatContext and AVStreams are not
       freed by this function */
    int (*read_close)(struct AVFormatContext *);
    /**
     * Seek to a given timestamp relative to the frames in
     * stream component stream_index.
     * @param stream_index must not be -1
     * @param flags selects which direction should be preferred if no exact
     *              match is available
     * @return >= 0 on success (but not necessarily the new offset)
     */
    int (*read_seek)(struct AVFormatContext *,
                     int stream_index, int64_t timestamp, int flags);
    /**
     * Gets the next timestamp in stream[stream_index].time_base units.
     * @return the timestamp or AV_NOPTS_VALUE if an error occurred
     */
    int64_t (*read_timestamp)(struct AVFormatContext *s, int stream_index,
                              int64_t *pos, int64_t pos_limit);
    /** Can use flags: AVFMT_NOFILE, AVFMT_NEEDNUMBER. */
    int flags;
    /** If extensions are defined, then no probe is done. You should
       usually not use extension format guessing because it is not
       reliable enough */
    const char *extensions;
    /** General purpose read-only value that the format can use. */
    int value;

    /** Start/resume playing - only meaningful if using a network-based format
       (RTSP). */
    int (*read_play)(struct AVFormatContext *);

    /** Pause playing - only meaningful if using a network-based format
       (RTSP). */
    int (*read_pause)(struct AVFormatContext *);

    const struct AVCodecTag * const *codec_tag;

    /* private fields */
    struct AVInputFormat *next;
} AVInputFormat;

enum AVStreamParseType {
    AVSTREAM_PARSE_NONE,
    AVSTREAM_PARSE_FULL,       /**< full parsing and repack */
    AVSTREAM_PARSE_HEADERS,    /**< Only parse headers, do not repack. */
    AVSTREAM_PARSE_TIMESTAMPS, /**< full parsing and interpolation of timestamps for frames not starting on a packet boundary */
};

typedef struct AVIndexEntry {
    int64_t pos;
    int64_t timestamp;
#define AVINDEX_KEYFRAME 0x0001
    int flags:2;
    int size:30; //Yeah, trying to keep the size of this small to reduce memory requirements (it is 24 vs. 32 bytes due to possible 8-byte alignment).
    int min_distance;         /**< Minimum distance between this and the previous keyframe, used to avoid unneeded searching. */
} AVIndexEntry;

#define AV_DISPOSITION_DEFAULT   0x0001
#define AV_DISPOSITION_DUB       0x0002
#define AV_DISPOSITION_ORIGINAL  0x0004
#define AV_DISPOSITION_COMMENT   0x0008
#define AV_DISPOSITION_LYRICS    0x0010
#define AV_DISPOSITION_KARAOKE   0x0020

/**
 * Stream structure.
 * New fields can be added to the end with minor version bumps.
 * Removal, reordering and changes to existing fields require a major
 * version bump.
 * sizeof(AVStream) must not be used outside libav*.
 */
typedef struct AVStream {
    int index;    /**< stream index in AVFormatContext */
    int id;       /**< format-specific stream ID */
    AVCodecContext *codec; /**< codec context */
    /**
     * Real base frame rate of the stream.
     * This is the lowest frame rate with which all timestamps can be
     * represented accurately (it is the least common multiple of all
     * frame rates in the stream). Note, this value is just a guess!
     * For example if the time base is 1/90000 and all frames have either
     * approximately 3600 or 1800 timer ticks, then r_frame_rate will be 50/1.
     */
    AVRational r_frame_rate;
    void *priv_data;

    /* internal data used in av_find_stream_info() */
    int64_t first_dts;
    /** encoding: pts generation when outputting stream */
    struct AVFrac pts;

    /**
     * This is the fundamental unit of time (in seconds) in terms
     * of which frame timestamps are represented. For fixed-fps content,
     * time base should be 1/frame rate and timestamp increments should be 1.
     */
    AVRational time_base;
    int pts_wrap_bits; /**< number of bits in pts (used for wrapping control) */
    /* ffmpeg.c private use */
    int stream_copy; /**< If set, just copy stream. */
    enum AVDiscard discard; ///< Selects which packets can be discarded at will and do not need to be demuxed.
    //FIXME move stuff to a flags field?
    /** Quality, as it has been removed from AVCodecContext and put in AVVideoFrame.
     * MN: dunno if that is the right place for it */
    float quality;
    /**
     * Decoding: pts of the first frame of the stream, in stream time base.
     * Only set this if you are absolutely 100% sure that the value you set
     * it to really is the pts of the first frame.
     * This may be undefined (AV_NOPTS_VALUE).
     * @note The ASF header does NOT contain a correct start_time the ASF
     * demuxer must NOT set this.
     */
    int64_t start_time;
    /**
     * Decoding: duration of the stream, in stream time base.
     * If a source file does not specify a duration, but does specify
     * a bitrate, this value will be estimated from bitrate and file size.
     */
    int64_t duration;

    char language[4]; /** ISO 639 3-letter language code (empty string if undefined) */

    /* av_read_frame() support */
    enum AVStreamParseType need_parsing;
    struct AVCodecParserContext *parser;

    int64_t cur_dts;
    int last_IP_duration;
    int64_t last_IP_pts;
    /* av_seek_frame() support */
    AVIndexEntry *index_entries; /**< Only used if the format does not
                                    support seeking natively. */
    int nb_index_entries;
    unsigned int index_entries_allocated_size;

    int64_t nb_frames;                 ///< number of frames in this stream if known or 0

#if LIBAVFORMAT_VERSION_INT < (53<<16)
    int64_t unused[4+1];
#endif

    char *filename; /**< source filename of the stream */

    int disposition; /**< AV_DISPOSITION_* bit field */

    AVProbeData probe_data;
#define MAX_REORDER_DELAY 16
    int64_t pts_buffer[MAX_REORDER_DELAY+1];

    /**
     * sample aspect ratio (0 if unknown)
     * - encoding: Set by user.
     * - decoding: Set by libavformat.
     */
    AVRational sample_aspect_ratio;

    AVMetadata *metadata;
} AVStream;

#define AV_PROGRAM_RUNNING 1

/**
 * New fields can be added to the end with minor version bumps.
 * Removal, reordering and changes to existing fields require a major
 * version bump.
 * sizeof(AVProgram) must not be used outside libav*.
 */
typedef struct AVProgram {
    int            id;
    char           *provider_name; ///< network name for DVB streams
    char           *name;          ///< service name for DVB streams
    int            flags;
    enum AVDiscard discard;        ///< selects which program to discard and which to feed to the caller
    unsigned int   *stream_index;
    unsigned int   nb_stream_indexes;
    AVMetadata *metadata;
} AVProgram;

#define AVFMTCTX_NOHEADER      0x0001 /**< signal that no header is present
                                         (streams are added dynamically) */

typedef struct AVChapter {
    int id;                 ///< unique ID to identify the chapter
    AVRational time_base;   ///< time base in which the start/end timestamps are specified
    int64_t start, end;     ///< chapter start/end time in time_base units
    char *title;            ///< chapter title
    AVMetadata *metadata;
} AVChapter;

#define MAX_STREAMS 20

/**
 * Format I/O context.
 * New fields can be added to the end with minor version bumps.
 * Removal, reordering and changes to existing fields require a major
 * version bump.
 * sizeof(AVFormatContext) must not be used outside libav*.
 */
typedef struct AVFormatContext {
    const AVClass *av_class; /**< Set by av_alloc_format_context. */
    /* Can only be iformat or oformat, not both at the same time. */
    struct AVInputFormat *iformat;
    struct AVOutputFormat *oformat;
    void *priv_data;
    ByteIOContext *pb;
    unsigned int nb_streams;
    AVStream *streams[MAX_STREAMS];
    char filename[1024]; /**< input or output filename */
    /* stream info */
    int64_t timestamp;
    char title[512];
    char author[512];
    char copyright[512];
    char comment[512];
    char album[512];
    int year;  /**< ID3 year, 0 if none */
    int track; /**< track number, 0 if none */
    char genre[32]; /**< ID3 genre */

    int ctx_flags; /**< Format-specific flags, see AVFMTCTX_xx */
    /* private data for pts handling (do not modify directly). */
    /** This buffer is only needed when packets were already buffered but
       not decoded, for example to get the codec parameters in MPEG
       streams. */
    struct AVPacketList *packet_buffer;

    /** Decoding: position of the first frame of the component, in
       AV_TIME_BASE fractional seconds. NEVER set this value directly:
       It is deduced from the AVStream values.  */
    int64_t start_time;
    /** Decoding: duration of the stream, in AV_TIME_BASE fractional
       seconds. NEVER set this value directly: it is deduced from the
       AVStream values.  */
    int64_t duration;
    /** decoding: total file size, 0 if unknown */
    int64_t file_size;
    /** Decoding: total stream bitrate in bit/s, 0 if not
       available. Never set it directly if the file_size and the
       duration are known as ffmpeg can compute it automatically. */
    int bit_rate;

    /* av_read_frame() support */
    AVStream *cur_st;
    const uint8_t *cur_ptr;
    int cur_len;
    AVPacket cur_pkt;

    /* av_seek_frame() support */
    int64_t data_offset; /** offset of the first packet */
    int index_built;

    int mux_rate;
    int packet_size;
    int preload;
    int max_delay;

#define AVFMT_NOOUTPUTLOOP -1
#define AVFMT_INFINITEOUTPUTLOOP 0
    /** number of times to loop output in formats that support it */
    int loop_output;

    int flags;
#define AVFMT_FLAG_GENPTS       0x0001 ///< Generate pts if missing even if it requires parsing future frames.
#define AVFMT_FLAG_IGNIDX       0x0002 ///< Ignore index.
#define AVFMT_FLAG_NONBLOCK     0x0004 ///< Do not block when reading packets from input.

    int loop_input;
    /** Decoding: size of data to probe; encoding: unused. */
    unsigned int probesize;

    /**
     * Maximum time (in AV_TIME_BASE units) during which the input should
     * be analyzed in av_find_stream_info().
     */
    int max_analyze_duration;

    const uint8_t *key;
    int keylen;

    unsigned int nb_programs;
    AVProgram **programs;

    /**
     * Forced video codec_id.
     * Demuxing: Set by user.
     */
    enum CodecID video_codec_id;
    /**
     * Forced audio codec_id.
     * Demuxing: Set by user.
     */
    enum CodecID audio_codec_id;
    /**
     * Forced subtitle codec_id.
     * Demuxing: Set by user.
     */
    enum CodecID subtitle_codec_id;

    /**
     * Maximum amount of memory in bytes to use per stream for the index.
     * If the needed index exceeds this size, entries will be discarded as
     * needed to maintain a smaller size. This can lead to slower or less
     * accurate seeking (depends on demuxer).
     * Demuxers for which a full in-memory index is mandatory will ignore
     * this.
     * muxing  : unused
     * demuxing: set by user
     */
    unsigned int max_index_size;

    /**
     * Maximum amount of memory in bytes to use for buffering frames
     * obtained from realtime capture devices.
     */
    unsigned int max_picture_buffer;

    unsigned int nb_chapters;
    AVChapter **chapters;

    /**
     * Flags to enable debugging.
     */
    int debug;
#define FF_FDEBUG_TS        0x0001

    /**
     * Raw packets from the demuxer, prior to parsing and decoding.
     * This buffer is used for buffering packets until the codec can
     * be identified, as parsing cannot be done without knowing the
     * codec.
     */
    struct AVPacketList *raw_packet_buffer;
    struct AVPacketList *raw_packet_buffer_end;

    struct AVPacketList *packet_buffer_end;

    AVMetadata *metadata;
} AVFormatContext;

typedef struct AVPacketList {
    AVPacket pkt;
    struct AVPacketList *next;
} AVPacketList;

#if LIBAVFORMAT_VERSION_INT < (53<<16)
extern AVInputFormat *first_iformat;
extern AVOutputFormat *first_oformat;
#endif

AVInputFormat  *av_iformat_next(AVInputFormat  *f);
AVOutputFormat *av_oformat_next(AVOutputFormat *f);

enum CodecID av_guess_image2_codec(const char *filename);

/* XXX: use automatic init with either ELF sections or C file parser */
/* modules */

/* utils.c */
void av_register_input_format(AVInputFormat *format);
void av_register_output_format(AVOutputFormat *format);
AVOutputFormat *guess_stream_format(const char *short_name,
                                    const char *filename,
                                    const char *mime_type);
AVOutputFormat *guess_format(const char *short_name,
                             const char *filename,
                             const char *mime_type);

/**
 * Guesses the codec ID based upon muxer and filename.
 */
enum CodecID av_guess_codec(AVOutputFormat *fmt, const char *short_name,
                            const char *filename, const char *mime_type,
                            enum CodecType type);

/**
 * Send a nice hexadecimal dump of a buffer to the specified file stream.
 *
 * @param f The file stream pointer where the dump should be sent to.
 * @param buf buffer
 * @param size buffer size
 *
 * @see av_hex_dump_log, av_pkt_dump, av_pkt_dump_log
 */
void av_hex_dump(FILE *f, uint8_t *buf, int size);

/**
 * Send a nice hexadecimal dump of a buffer to the log.
 *
 * @param avcl A pointer to an arbitrary struct of which the first field is a
 * pointer to an AVClass struct.
 * @param level The importance level of the message, lower values signifying
 * higher importance.
 * @param buf buffer
 * @param size buffer size
 *
 * @see av_hex_dump, av_pkt_dump, av_pkt_dump_log
 */
void av_hex_dump_log(void *avcl, int level, uint8_t *buf, int size);

/**
 * Send a nice dump of a packet to the specified file stream.
 *
 * @param f The file stream pointer where the dump should be sent to.
 * @param pkt packet to dump
 * @param dump_payload True if the payload must be displayed, too.
 */
void av_pkt_dump(FILE *f, AVPacket *pkt, int dump_payload);

/**
 * Send a nice dump of a packet to the log.
 *
 * @param avcl A pointer to an arbitrary struct of which the first field is a
 * pointer to an AVClass struct.
 * @param level The importance level of the message, lower values signifying
 * higher importance.
 * @param pkt packet to dump
 * @param dump_payload True if the payload must be displayed, too.
 */
void av_pkt_dump_log(void *avcl, int level, AVPacket *pkt, int dump_payload);

void av_register_all(void);

/** codec tag <-> codec id */
enum CodecID av_codec_get_id(const struct AVCodecTag * const *tags, unsigned int tag);
unsigned int av_codec_get_tag(const struct AVCodecTag * const *tags, enum CodecID id);

/* media file input */

/**
 * Finds AVInputFormat based on the short name of the input format.
 */
AVInputFormat *av_find_input_format(const char *short_name);

/**
 * Guess file format.
 *
 * @param is_opened Whether the file is already opened; determines whether
 *                  demuxers with or without AVFMT_NOFILE are probed.
 */
AVInputFormat *av_probe_input_format(AVProbeData *pd, int is_opened);

/**
 * Allocates all the structures needed to read an input stream.
 *        This does not open the needed codecs for decoding the stream[s].
 */
int av_open_input_stream(AVFormatContext **ic_ptr,
                         ByteIOContext *pb, const char *filename,
                         AVInputFormat *fmt, AVFormatParameters *ap);

/**
 * Open a media file as input. The codecs are not opened. Only the file
 * header (if present) is read.
 *
 * @param ic_ptr The opened media file handle is put here.
 * @param filename filename to open
 * @param fmt If non-NULL, force the file format to use.
 * @param buf_size optional buffer size (zero if default is OK)
 * @param ap Additional parameters needed when opening the file
 *           (NULL if default).
 * @return 0 if OK, AVERROR_xxx otherwise
 */
int av_open_input_file(AVFormatContext **ic_ptr, const char *filename,
                       AVInputFormat *fmt,
                       int buf_size,
                       AVFormatParameters *ap);
/**
 * Allocate an AVFormatContext.
 * Can be freed with av_free() but do not forget to free everything you
 * explicitly allocated as well!
 */
AVFormatContext *av_alloc_format_context(void);

/**
 * Read packets of a media file to get stream information. This
 * is useful for file formats with no headers such as MPEG. This
 * function also computes the real frame rate in case of MPEG-2 repeat
 * frame mode.
 * The logical file position is not changed by this function;
 * examined packets may be buffered for later processing.
 *
 * @param ic media file handle
 * @return >=0 if OK, AVERROR_xxx on error
 * @todo Let the user decide somehow what information is needed so that
 *       we do not waste time getting stuff the user does not need.
 */
int av_find_stream_info(AVFormatContext *ic);

/**
 * Read a transport packet from a media file.
 *
 * This function is obsolete and should never be used.
 * Use av_read_frame() instead.
 *
 * @param s media file handle
 * @param pkt is filled
 * @return 0 if OK, AVERROR_xxx on error
 */
int av_read_packet(AVFormatContext *s, AVPacket *pkt);

/**
 * Return the next frame of a stream.
 *
 * The returned packet is valid
 * until the next av_read_frame() or until av_close_input_file() and
 * must be freed with av_free_packet. For video, the packet contains
 * exactly one frame. For audio, it contains an integer number of
 * frames if each frame has a known fixed size (e.g. PCM or ADPCM
 * data). If the audio frames have a variable size (e.g. MPEG audio),
 * then it contains one frame.
 *
 * pkt->pts, pkt->dts and pkt->duration are always set to correct
 * values in AVStream.timebase units (and guessed if the format cannot
 * provide them). pkt->pts can be AV_NOPTS_VALUE if the video format
 * has B-frames, so it is better to rely on pkt->dts if you do not
 * decompress the payload.
 *
 * @return 0 if OK, < 0 on error or end of file
 */
int av_read_frame(AVFormatContext *s, AVPacket *pkt);

/**
 * Seek to the key frame at timestamp.
 * 'timestamp' in 'stream_index'.
 * @param stream_index If stream_index is (-1), a default
 * stream is selected, and timestamp is automatically converted
 * from AV_TIME_BASE units to the stream specific time_base.
 * @param timestamp Timestamp in AVStream.time_base units
 *        or, if no stream is specified, in AV_TIME_BASE units.
 * @param flags flags which select direction and seeking mode
 * @return >= 0 on success
 */
int av_seek_frame(AVFormatContext *s, int stream_index, int64_t timestamp,
                  int flags);

/**
 * Start playing a network based stream (e.g. RTSP stream) at the
 * current position.
 */
int av_read_play(AVFormatContext *s);

/**
 * Pause a network based stream (e.g. RTSP stream).
 *
 * Use av_read_play() to resume it.
 */
int av_read_pause(AVFormatContext *s);

/**
 * Free a AVFormatContext allocated by av_open_input_stream.
 * @param s context to free
 */
void av_close_input_stream(AVFormatContext *s);

/**
 * Close a media file (but not its codecs).
 *
 * @param s media file handle
 */
void av_close_input_file(AVFormatContext *s);

/**
 * Add a new stream to a media file.
 *
 * Can only be called in the read_header() function. If the flag
 * AVFMTCTX_NOHEADER is in the format context, then new streams
 * can be added in read_packet too.
 *
 * @param s media file handle
 * @param id file-format-dependent stream ID
 */
AVStream *av_new_stream(AVFormatContext *s, int id);
AVProgram *av_new_program(AVFormatContext *s, int id);

/**
 * Add a new chapter.
 * This function is NOT part of the public API
 * and should ONLY be used by demuxers.
 *
 * @param s media file handle
 * @param id unique ID for this chapter
 * @param start chapter start time in time_base units
 * @param end chapter end time in time_base units
 * @param title chapter title
 *
 * @return AVChapter or NULL on error
 */
AVChapter *ff_new_chapter(AVFormatContext *s, int id, AVRational time_base,
                          int64_t start, int64_t end, const char *title);

/**
 * Set the pts for a given stream.
 *
 * @param s stream
 * @param pts_wrap_bits number of bits effectively used by the pts
 *        (used for wrap control, 33 is the value for MPEG)
 * @param pts_num numerator to convert to seconds (MPEG: 1)
 * @param pts_den denominator to convert to seconds (MPEG: 90000)
 */
void av_set_pts_info(AVStream *s, int pts_wrap_bits,
                     int pts_num, int pts_den);

#define AVSEEK_FLAG_BACKWARD 1 ///< seek backward
#define AVSEEK_FLAG_BYTE     2 ///< seeking based on position in bytes
#define AVSEEK_FLAG_ANY      4 ///< seek to any frame, even non-keyframes

int av_find_default_stream_index(AVFormatContext *s);

/**
 * Gets the index for a specific timestamp.
 * @param flags if AVSEEK_FLAG_BACKWARD then the returned index will correspond
 *                 to the timestamp which is <= the requested one, if backward
 *                 is 0, then it will be >=
 *              if AVSEEK_FLAG_ANY seek to any frame, only keyframes otherwise
 * @return < 0 if no such timestamp could be found
 */
int av_index_search_timestamp(AVStream *st, int64_t timestamp, int flags);

/**
 * Ensures the index uses less memory than the maximum specified in
 * AVFormatContext.max_index_size, by discarding entries if it grows
 * too large.
 * This function is not part of the public API and should only be called
 * by demuxers.
 */
void ff_reduce_index(AVFormatContext *s, int stream_index);

/**
 * Add an index entry into a sorted list. Update the entry if the list
 * already contains it.
 *
 * @param timestamp timestamp in the time base of the given stream
 */
int av_add_index_entry(AVStream *st, int64_t pos, int64_t timestamp,
                       int size, int distance, int flags);

/**
 * Does a binary search using av_index_search_timestamp() and
 * AVCodec.read_timestamp().
 * This is not supposed to be called directly by a user application,
 * but by demuxers.
 * @param target_ts target timestamp in the time base of the given stream
 * @param stream_index stream number
 */
int av_seek_frame_binary(AVFormatContext *s, int stream_index,
                         int64_t target_ts, int flags);

/**
 * Updates cur_dts of all streams based on the given timestamp and AVStream.
 *
 * Stream ref_st unchanged, others set cur_dts in their native time base.
 * Only needed for timestamp wrapping or if (dts not set and pts!=dts).
 * @param timestamp new dts expressed in time_base of param ref_st
 * @param ref_st reference stream giving time_base of param timestamp
 */
void av_update_cur_dts(AVFormatContext *s, AVStream *ref_st, int64_t timestamp);

/**
 * Does a binary search using read_timestamp().
 * This is not supposed to be called directly by a user application,
 * but by demuxers.
 * @param target_ts target timestamp in the time base of the given stream
 * @param stream_index stream number
 */
int64_t av_gen_search(AVFormatContext *s, int stream_index,
                      int64_t target_ts, int64_t pos_min,
                      int64_t pos_max, int64_t pos_limit,
                      int64_t ts_min, int64_t ts_max,
                      int flags, int64_t *ts_ret,
                      int64_t (*read_timestamp)(struct AVFormatContext *, int , int64_t *, int64_t ));

/** media file output */
int av_set_parameters(AVFormatContext *s, AVFormatParameters *ap);

/**
 * Allocate the stream private data and write the stream header to an
 * output media file.
 *
 * @param s media file handle
 * @return 0 if OK, AVERROR_xxx on error
 */
int av_write_header(AVFormatContext *s);

/**
 * Write a packet to an output media file.
 *
 * The packet shall contain one audio or video frame.
 * The packet must be correctly interleaved according to the container
 * specification, if not then av_interleaved_write_frame must be used.
 *
 * @param s media file handle
 * @param pkt The packet, which contains the stream_index, buf/buf_size,
              dts/pts, ...
 * @return < 0 on error, = 0 if OK, 1 if end of stream wanted
 */
int av_write_frame(AVFormatContext *s, AVPacket *pkt);

/**
 * Writes a packet to an output media file ensuring correct interleaving.
 *
 * The packet must contain one audio or video frame.
 * If the packets are already correctly interleaved the application should
 * call av_write_frame() instead as it is slightly faster. It is also important
 * to keep in mind that completely non-interleaved input will need huge amounts
 * of memory to interleave with this, so it is preferable to interleave at the
 * demuxer level.
 *
 * @param s media file handle
 * @param pkt The packet, which contains the stream_index, buf/buf_size,
              dts/pts, ...
 * @return < 0 on error, = 0 if OK, 1 if end of stream wanted
 */
int av_interleaved_write_frame(AVFormatContext *s, AVPacket *pkt);

/**
 * Interleave a packet per dts in an output media file.
 *
 * Packets with pkt->destruct == av_destruct_packet will be freed inside this
 * function, so they cannot be used after it, note calling av_free_packet()
 * on them is still safe.
 *
 * @param s media file handle
 * @param out the interleaved packet will be output here
 * @param in the input packet
 * @param flush 1 if no further packets are available as input and all
 *              remaining packets should be output
 * @return 1 if a packet was output, 0 if no packet could be output,
 *         < 0 if an error occurred
 */
int av_interleave_packet_per_dts(AVFormatContext *s, AVPacket *out,
                                 AVPacket *pkt, int flush);

/**
 * @brief Write the stream trailer to an output media file and
 *        free the file private data.
 *
 * May only be called after a successful call to av_write_header.
 *
 * @param s media file handle
 * @return 0 if OK, AVERROR_xxx on error
 */
int av_write_trailer(AVFormatContext *s);

void dump_format(AVFormatContext *ic,
                 int index,
                 const char *url,
                 int is_output);

#if LIBAVFORMAT_VERSION_MAJOR < 53
/**
 * Parses width and height out of string str.
 * @deprecated Use av_parse_video_frame_size instead.
 */
attribute_deprecated int parse_image_size(int *width_ptr, int *height_ptr,
                                          const char *str);

/**
 * Converts frame rate from string to a fraction.
 * @deprecated Use av_parse_video_frame_rate instead.
 */
attribute_deprecated int parse_frame_rate(int *frame_rate, int *frame_rate_base,
                                          const char *arg);
#endif

/**
 * Parses \p datestr and returns a corresponding number of microseconds.
 * @param datestr String representing a date or a duration.
 * - If a date the syntax is:
 * @code
 *  [{YYYY-MM-DD|YYYYMMDD}]{T| }{HH[:MM[:SS[.m...]]][Z]|HH[MM[SS[.m...]]][Z]}
 * @endcode
 * Time is local time unless Z is appended, in which case it is
 * interpreted as UTC.
 * If the year-month-day part is not specified it takes the current
 * year-month-day.
 * Returns the number of microseconds since 1st of January, 1970 up to
 * the time of the parsed date or INT64_MIN if \p datestr cannot be
 * successfully parsed.
 * - If a duration the syntax is:
 * @code
 *  [-]HH[:MM[:SS[.m...]]]
 *  [-]S+[.m...]
 * @endcode
 * Returns the number of microseconds contained in a time interval
 * with the specified duration or INT64_MIN if \p datestr cannot be
 * successfully parsed.
 * @param duration Flag which tells how to interpret \p datestr, if
 * not zero \p datestr is interpreted as a duration, otherwise as a
 * date.
 */
int64_t parse_date(const char *datestr, int duration);

/** Gets the current time in microseconds. */
int64_t av_gettime(void);

/* ffm-specific for ffserver */
#define FFM_PACKET_SIZE 4096
int64_t ffm_read_write_index(int fd);
void ffm_write_write_index(int fd, int64_t pos);
void ffm_set_write_index(AVFormatContext *s, int64_t pos, int64_t file_size);

/**
 * Attempts to find a specific tag in a URL.
 *
 * syntax: '?tag1=val1&tag2=val2...'. Little URL decoding is done.
 * Return 1 if found.
 */
int find_info_tag(char *arg, int arg_size, const char *tag1, const char *info);

/**
 * Returns in 'buf' the path with '%d' replaced by number.
 *
 * Also handles the '%0nd' format where 'n' is the total number
 * of digits and '%%'.
 *
 * @param buf destination buffer
 * @param buf_size destination buffer size
 * @param path numbered sequence string
 * @param number frame number
 * @return 0 if OK, -1 on format error
 */
int av_get_frame_filename(char *buf, int buf_size,
                          const char *path, int number);

/**
 * Check whether filename actually is a numbered sequence generator.
 *
 * @param filename possible numbered sequence string
 * @return 1 if a valid numbered sequence string, 0 otherwise
 */
int av_filename_number_test(const char *filename);

/**
 * Generate an SDP for an RTP session.
 *
 * @param ac array of AVFormatContexts describing the RTP streams. If the
 *           array is composed by only one context, such context can contain
 *           multiple AVStreams (one AVStream per RTP stream). Otherwise,
 *           all the contexts in the array (an AVCodecContext per RTP stream)
 *           must contain only one AVStream.
 * @param n_files number of AVCodecContexts contained in ac
 * @param buff buffer where the SDP will be stored (must be allocated by
 *             the caller)
 * @param size the size of the buffer
 * @return 0 if OK, AVERROR_xxx on error
 */
int avf_sdp_create(AVFormatContext *ac[], int n_files, char *buff, int size);

#ifdef HAVE_AV_CONFIG_H

void ff_dynarray_add(unsigned long **tab_ptr, int *nb_ptr, unsigned long elem);

#ifdef __GNUC__
#define dynarray_add(tab, nb_ptr, elem)\
do {\
    __typeof__(tab) _tab = (tab);\
    __typeof__(elem) _elem = (elem);\
    (void)sizeof(**_tab == _elem); /* check that types are compatible */\
    ff_dynarray_add((unsigned long **)_tab, nb_ptr, (unsigned long)_elem);\
} while(0)
#else
#define dynarray_add(tab, nb_ptr, elem)\
do {\
    ff_dynarray_add((unsigned long **)(tab), nb_ptr, (unsigned long)(elem));\
} while(0)
#endif

time_t mktimegm(struct tm *tm);
struct tm *brktimegm(time_t secs, struct tm *tm);
const char *small_strptime(const char *p, const char *fmt,
                           struct tm *dt);

struct in_addr;
int resolve_host(struct in_addr *sin_addr, const char *hostname);

void url_split(char *proto, int proto_size,
               char *authorization, int authorization_size,
               char *hostname, int hostname_size,
               int *port_ptr,
               char *path, int path_size,
               const char *url);

int match_ext(const char *filename, const char *extensions);

#endif /* HAVE_AV_CONFIG_H */

#endif /* AVFORMAT_AVFORMAT_H */
