#!/bin/bash -p

# Copyright (c) 2009 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# usage: keystone_install.sh update_dmg_mount_point
#
# Called by the Keystone system to update the installed application with a new
# version from a disk image.
#
# Environment variables:
# GOOGLE_CHROME_UPDATER_DEBUG
#   When set to a non-empty value, additional information about this script's
#   actions will be logged to stderr.  The same debugging information will
#   also be enabled when "Library/Google/Google Chrome Updater Debug" in the
#   root directory or in ${HOME} exists.
#
# Exit codes:
#  0  Happiness
#  1  Unknown failure
#  2  Basic sanity check source failure (e.g. no app on disk image)
#  3  Basic sanity check destination failure (e.g. ticket points to nothing)
#  4  Update driven by user ticket when a system ticket is also present
#  5  Could not prepare existing installed version to receive update
#  6  rsync failed (could not assure presence of Versions directory)
#  7  rsync failed (could not copy new versioned directory to Versions)
#  8  rsync failed (could not update outer .app bundle)
#  9  Could not get the version, update URL, or channel after update
# 10  Updated application does not have the version number from the update
# 11  ksadmin failure

set -eu

# http://b/2290916: Keystone runs the installation with a restrictive PATH
# that only includes the directory containing ksadmin, /bin, and /usr/bin.  It
# does not include /sbin or /usr/sbin.  This script uses lsof, which is in
# /usr/sbin, and it's conceivable that it might want to use other tools in an
# sbin directory.  Adjust the path accordingly.
export PATH="${PATH}:/sbin:/usr/sbin"

# Environment sanitization.  Clear environment variables that might impact the
# interpreter's operation.  The |bash -p| invocation on the #! line takes the
# bite out of BASH_ENV, ENV, and SHELLOPTS (among other features), but
# clearing them here ensures that they won't impact any shell scripts used as
# utility programs. SHELLOPTS is read-only and can't be unset, only
# unexported.
unset BASH_ENV CDPATH ENV GLOBIGNORE IFS POSIXLY_CORRECT
export -n SHELLOPTS

set -o pipefail
shopt -s nullglob

ME="$(basename "${0}")"
readonly ME

declare GOOGLE_CHROME_UPDATER_DEBUG
err() {
  local error="${1}"

  local id=
  if [[ -n "${GOOGLE_CHROME_UPDATER_DEBUG}" ]]; then
    id=": ${$} $(date "+%Y-%m-%d %H:%M:%S %z")"
  fi

  echo "${ME}${id}: ${error}" >& 2
}

note() {
  local message="${1}"

  if [[ -n "${GOOGLE_CHROME_UPDATER_DEBUG}" ]]; then
    err "${message}"
  fi
}

# Returns 0 (true) if |symlink| exists, is a symbolic link, and appears
# writable on the basis of its POSIX permissions.  This is used to determine
# writability like test's -w primary, but -w resolves symbolic links and this
# function does not.
is_writable_symlink() {
  local symlink="${1}"

  local link_mode
  link_mode="$(stat -f %Sp "${symlink}" 2> /dev/null || true)"
  if [[ -z "${link_mode}" ]] || [[ "${link_mode:0:1}" != "l" ]]; then
    return 1
  fi

  local link_user link_group
  link_user="$(stat -f %u "${symlink}" 2> /dev/null || true)"
  link_group="$(stat -f %g "${symlink}" 2> /dev/null || true)"
  if [[ -z "${link_user}" ]] || [[ -z "${link_group}" ]]; then
    return 1
  fi

  # If the users match, check the owner-write bit.
  if [[ ${EUID} -eq "${link_user}" ]]; then
    if [[ "${link_mode:2:1}" = "w" ]]; then
      return 0
    fi
    return 1
  fi

  # If the file's group matches any of the groups that this process is a
  # member of, check the group-write bit.
  local group_match=
  local group
  for group in "${GROUPS[@]}"; do
    if [[ "${group}" -eq "${link_group}" ]]; then
      group_match="y"
      break
    fi
  done
  if [[ -n "${group_match}" ]]; then
    if [[ "${link_mode:5:1}" = "w" ]]; then
      return 0
    fi
    return 1
  fi

  # Check the other-write bit.
  if [[ "${link_mode:8:1}" = "w" ]]; then
    return 0
  fi

  return 1
}

# If |symlink| exists and is a symbolic link, but is not writable according to
# is_writable_symlink, this function attempts to replace it with a new
# writable symbolic link.  If |symlink| does not exist, is not a symbolic
# link, or is already writable, this function does nothing.  This function
# always returns 0 (true).
ensure_writable_symlink() {
  local symlink="${1}"

  if [[ -L "${symlink}" ]] && ! is_writable_symlink "${symlink}"; then
    # If ${symlink} refers to a directory, doing this naively might result in
    # the new link being placed in that directory, instead of replacing the
    # existing link.  ln -fhs is supposed to handle this case, but it does so
    # by unlinking (removing) the existing symbolic link before creating a new
    # one.  That leaves a small window during which the symbolic link is not
    # present on disk at all.
    #
    # To avoid that possibility, a new symbolic link is created in a temporary
    # location and then swapped into place with mv.  An extra temporary
    # directory is used to convince mv to replace the symbolic link: again, if
    # the existing link refers to a directory, "mv newlink oldlink" will
    # actually leave oldlink alone and place newlink into the directory.
    # "mv newlink dirname(oldlink)" works as expected, but in order to replace
    # oldlink, newlink must have the same basename, hence the temporary
    # directory.

    local target
    target="$(readlink "${symlink}" 2> /dev/null || true)"
    if [[ -z "${target}" ]]; then
      return 0
    fi

    # Error handling strategy: if anything fails, such as the mktemp, ln,
    # chmod, or mv, ignore the failure and return 0 (success), leaving the
    # existing state with the non-writable symbolic link intact.  Failures
    # in this function will be difficult to understand and diagnose, and a
    # non-writable symbolic link is not necessarily fatal.  If something else
    # requires a writable symbolic link, allowing it to fail when a symbolic
    # link is not writable is easier to understand than bailing out of the
    # script on failure here.

    local symlink_dir temp_link_dir temp_link
    symlink_dir="$(dirname "${symlink}")"
    temp_link_dir="$(mktemp -d "${symlink_dir}/.symlink_temp.XXXXXX" || true)"
    if [[ -z "${temp_link_dir}" ]]; then
      return 0
    fi
    temp_link="${temp_link_dir}/$(basename "${symlink}")"

    (ln -fhs "${target}" "${temp_link}" && \
        chmod -h 755 "${temp_link}" && \
        mv -f "${temp_link}" "${symlink_dir}/") || true
    rm -rf "${temp_link_dir}"
  fi

  return 0
}

# Prints the version of ksadmin, as reported by ksadmin --ksadmin-version, to
# stdout.  This function operates with "static" variables: it will only check
# the ksadmin version once per script run.  If ksadmin is old enough to not
# support --ksadmin-version, or another error occurs, this function prints an
# empty string.
g_checked_ksadmin_version=
g_ksadmin_version=
ksadmin_version() {
  if [[ -z "${g_checked_ksadmin_version}" ]]; then
    g_checked_ksadmin_version="y"
    g_ksadmin_version="$(ksadmin --ksadmin-version || true)"
  fi
  echo "${g_ksadmin_version}"
  return 0
}

# Compares the installed ksadmin version against a supplied version number,
# |check_version|, and returns 0 (true) if the installed Keystone version is
# greater than or equal to |check_version| according to a piece-wise
# comparison.  Returns 1 (false) if the installed Keystone version number
# cannot be determined or if |check_version| is greater than the installed
# Keystone version.  |check_version| should be a string of the form
# "major.minor.micro.build".  Returns 1 (false) if either |check_version| or
# the Keystone version do not match this format.
readonly VER_RE="^([0-9]+)\\.([0-9]+)\\.([0-9]+)\\.([0-9]+)\$"
is_ksadmin_version_ge() {
  local check_version="${1}"

  if ! [[ "${check_version}" =~ ${VER_RE} ]]; then
    return 1
  fi

  local check_components=("${BASH_REMATCH[1]}"
                          "${BASH_REMATCH[2]}"
                          "${BASH_REMATCH[3]}"
                          "${BASH_REMATCH[4]}")

  local ksadmin_version
  ksadmin_version="$(ksadmin_version)"

  if ! [[ "${ksadmin_version}" =~ ${VER_RE} ]]; then
    return 1
  fi

  local ksadmin_components=("${BASH_REMATCH[1]}"
                            "${BASH_REMATCH[2]}"
                            "${BASH_REMATCH[3]}"
                            "${BASH_REMATCH[4]}")

  local i
  for i in 0 1 2 3; do
    local check_component="${check_components[${i}]}"
    local ksadmin_component="${ksadmin_components[${i}]}"

    if [[ ${ksadmin_component} -lt ${check_component} ]]; then
      # ksadmin_version is less than check_version
      return 1
    fi
    if [[ ${ksadmin_component} -gt ${check_component} ]]; then
      # ksadmin_version is greater than check_version
      return 0
    fi
  done

  # The version numbers are equal.
  return 0
}

# Returns 0 (true) if ksadmin supports --tag.
ksadmin_supports_tag() {
  local ksadmin_version

  ksadmin_version="$(ksadmin_version)"
  if [[ -n "${ksadmin_version}" ]]; then
    # A ksadmin that recognizes --ksadmin-version and provides a version
    # number is new enough to recognize --tag.
    return 0
  fi

  return 1
}

# Returns 0 (true) if ksadmin supports --tag-path and --tag-key.
ksadmin_supports_tagpath_tagkey() {
  # --tag-path and --tag-key were introduced in Keystone 1.0.7.1306.
  is_ksadmin_version_ge 1.0.7.1306

  # The return value of is_ksadmin_version_ge is used as this function's
  # return value.
}

# Returns 0 (true) if ksadmin supports --brand-path and --brand-key.
ksadmin_supports_brandpath_brandkey() {
  # --brand-path and --brand-key were introduced in Keystone 1.0.8.1620.
  is_ksadmin_version_ge 1.0.8.1620

  # The return value of is_ksadmin_version_ge is used as this function's
  # return value.
}

usage() {
  echo "usage: ${ME} update_dmg_mount_point" >& 2
}

main() {
  local update_dmg_mount_point="${1}"

  # Early steps are critical.  Don't continue past any failure.
  set -e

  readonly PRODUCT_NAME="Google Chrome"
  readonly APP_DIR="${PRODUCT_NAME}.app"
  readonly FRAMEWORK_NAME="${PRODUCT_NAME} Framework"
  readonly FRAMEWORK_DIR="${FRAMEWORK_NAME}.framework"
  readonly CONTENTS_DIR="Contents"
  readonly APP_PLIST="${CONTENTS_DIR}/Info"
  readonly VERSIONS_DIR="${CONTENTS_DIR}/Versions"
  readonly UNROOTED_BRAND_PLIST="Library/Google/Google Chrome Brand"
  readonly UNROOTED_DEBUG_FILE="Library/Google/Google Chrome Updater Debug"

  readonly APP_VERSION_KEY="CFBundleShortVersionString"
  readonly KS_VERSION_KEY="KSVersion"
  readonly KS_PRODUCT_KEY="KSProductID"
  readonly KS_URL_KEY="KSUpdateURL"
  readonly KS_CHANNEL_KEY="KSChannelID"
  readonly KS_BRAND_KEY="KSBrandID"

  readonly QUARANTINE_ATTR="com.apple.quarantine"

  # Don't use rsync -a, because -a expands to -rlptgoD.  -g and -o copy owners
  # and groups, respectively, from the source, and that is undesirable in this
  # case.  -D copies devices and special files; copying devices only works
  # when running as root, so for consistency between privileged and
  # unprivileged operation, this option is omitted as well.
  #  -I, --ignore-times  don't skip files that match in size and mod-time
  #  -l, --links         copy symlinks as symlinks
  #  -r, --recursive     recurse into directories
  #  -p, --perms         preserve permissions
  #  -t, --times         preserve times
  readonly RSYNC_FLAGS="-Ilprt"

  # It's difficult to get GOOGLE_CHROME_UPDATER_DEBUG set in the environment
  # when this script is called from Keystone.  If a "debug file" exists in
  # either the root directory or the home directory of the user who owns the
  # ticket, turn on verbosity.  This may aid debugging.
  if [[ -e "/${UNROOTED_DEBUG_FILE}" ]] ||
     [[ -e ~/"${UNROOTED_DEBUG_FILE}" ]]; then
    export GOOGLE_CHROME_UPDATER_DEBUG="y"
  fi

  note "update_dmg_mount_point = ${update_dmg_mount_point}"

  # The argument should be the disk image path.  Make sure it exists and that
  # it's an absolute path.
  note "checking update"

  if [[ -z "${update_dmg_mount_point}" ]] ||
     [[ "${update_dmg_mount_point:0:1}" != "/" ]] ||
     ! [[ -d "${update_dmg_mount_point}" ]]; then
    err "update_dmg_mount_point must be an absolute path to a directory"
    usage
    exit 2
  fi

  # The update to install.
  local update_app="${update_dmg_mount_point}/${APP_DIR}"
  note "update_app = ${update_app}"

  # Make sure that there's something to copy from, and that it's an absolute
  # path.
  if [[ "${update_app:0:1}" != "/" ]] ||
     ! [[ -d "${update_app}" ]]; then
    err "update_app must be an absolute path to a directory"
    exit 2
  fi

  # Get some information about the update.
  note "reading update values"

  local update_app_plist="${update_app}/${APP_PLIST}"
  note "update_app_plist = ${update_app_plist}"
  local update_version_app
  if ! update_version_app="$(defaults read "${update_app_plist}" \
                                           "${APP_VERSION_KEY}")" ||
     [[ -z "${update_version_app}" ]]; then
    err "couldn't determine update_version_app"
    exit 2
  fi
  note "update_version_app = ${update_version_app}"

  local update_ks_plist="${update_app_plist}"
  note "update_ks_plist = ${update_ks_plist}"
  local update_version_ks
  if ! update_version_ks="$(defaults read "${update_ks_plist}" \
                                          "${KS_VERSION_KEY}")" ||
     [[ -z "${update_version_ks}" ]]; then
    err "couldn't determine update_version_ks"
    exit 2
  fi
  note "update_version_ks = ${update_version_ks}"

  local product_id
  if ! product_id="$(defaults read "${update_ks_plist}" \
                                   "${KS_PRODUCT_KEY}")" ||
     [[ -z "${product_id}" ]]; then
    err "couldn't determine product_id"
    exit 2
  fi
  note "product_id = ${product_id}"

  # ksadmin is required. Keystone should have set a ${PATH} that includes it.
  # Check that here, so that more useful feedback can be offered in the
  # unlikely event that ksadmin is missing.
  note "checking Keystone"

  local ksadmin_path
  if ! ksadmin_path="$(type -p ksadmin)" || [[ -z "${ksadmin_path}" ]]; then
    err "couldn't locate ksadmin_path"
    exit 3
  fi
  note "ksadmin_path = ${ksadmin_path}"

  # Call ksadmin_version once to prime the global state.  This is needed
  # because subsequent calls to ksadmin_version that occur in $(...)
  # expansions will not affect the global state (although they can read from
  # the already-initialized global state) and thus will cause a new ksadmin
  # --ksadmin-version process to run for each check unless the globals have
  # been properly initialized beforehand.
  ksadmin_version >& /dev/null || true
  local ksadmin_version_string
  ksadmin_version_string="$(ksadmin_version 2> /dev/null || true)"
  note "ksadmin_version_string = ${ksadmin_version_string}"

  # Figure out where to install.
  local installed_app
  if ! installed_app="$(ksadmin -pP "${product_id}" | sed -Ene \
      "s%^[[:space:]]+xc=<KSPathExistenceChecker:.* path=(/.+)>\$%\\1%p")" ||
      [[ -z "${installed_app}" ]]; then
    err "couldn't locate installed_app"
    exit 3
  fi
  note "installed_app = ${installed_app}"

  if [[ "${installed_app:0:1}" != "/" ]] ||
     ! [[ -d "${installed_app}" ]]; then
    err "installed_app must be an absolute path to a directory"
    exit 3
  fi

  # If this script is running as root, it's being driven by a system ticket.
  # Otherwise, it's being driven by a user ticket.
  local system_ticket=
  if [[ ${EUID} -eq 0 ]]; then
    system_ticket="y"
  fi
  note "system_ticket = ${system_ticket}"

  # If this script is being driven by a user ticket, but a system ticket is
  # also present, there's a potential for the two to collide.  Both ticket
  # types might be present if another user on the system promoted the ticket
  # to system: the other user could not have removed this user's user ticket.
  # Handle that case here by deleting the user ticket and exiting early with
  # a discrete exit code.
  #
  # Current versions of ksadmin will exit 1 (false) when asked to print tickets
  # and given a specific product ID to print.  Older versions of ksadmin would
  # exit 0 (true), but those same versions did not support -S (meaning to check
  # the system ticket store) and would exit 1 (false) with this invocation due
  # to not understanding the question.  Therefore, the usage here will only
  # delete the existing user ticket when running as non-root with access to a
  # sufficiently recent ksadmin.  Older ksadmins are tolerated: the update will
  # likely fail for another reason and the user ticket will hang around until
  # something is eventually able to remove it.
  if [[ -z "${system_ticket}" ]] &&
     ksadmin -S --print-tickets -P "${product_id}" >& /dev/null; then
    ksadmin --delete -P "${product_id}" || true
    err "can't update on a user ticket when a system ticket is also present"
    exit 4
  fi

  # Figure out what the existing installed application is using for its
  # versioned directory.  This will be used later, to avoid removing the
  # existing installed version's versioned directory in case anything is still
  # using it.
  note "reading install values"

  local installed_app_plist="${installed_app}/${APP_PLIST}"
  note "installed_app_plist = ${installed_app_plist}"
  local installed_app_plist_path="${installed_app_plist}.plist"
  note "installed_app_plist_path = ${installed_app_plist_path}"
  local old_version_app
  old_version_app="$(defaults read "${installed_app_plist}" \
                                   "${APP_VERSION_KEY}" || true)"
  note "old_version_app = ${old_version_app}"

  local installed_versions_dir="${installed_app}/${VERSIONS_DIR}"
  note "installed_versions_dir = ${installed_versions_dir}"

  # If the installed application is incredibly old, old_versioned_dir may not
  # exist.
  local old_versioned_dir
  if [[ -n "${old_version_app}" ]]; then
    old_versioned_dir="${installed_versions_dir}/${old_version_app}"
  fi
  note "old_versioned_dir = ${old_versioned_dir}"

  # Collect the installed application's brand code, it will be used later.  It
  # is not an error for the installed application to not have a brand code.
  local old_ks_plist="${installed_app_plist}"
  note "old_ks_plist = ${old_ks_plist}"
  local old_brand
  old_brand="$(defaults read "${old_ks_plist}" \
                             "${KS_BRAND_KEY}" 2> /dev/null ||
               true)"
  note "old_brand = ${old_brand}"

  # See if the timestamp of what's currently on disk is newer than the
  # update's outer .app's timestamp.  rsync will copy the update's timestamp
  # over, but if that timestamp isn't as recent as what's already on disk, the
  # .app will need to be touched.
  local needs_touch=
  if [[ "${installed_app}" -nt "${update_app}" ]]; then
    needs_touch="y"
  fi
  note "needs_touch = ${needs_touch}"

  # In some very weird and rare cases, it is possible to wind up with a user
  # installation that contains symbolic links that the user does not have
  # write permission over.  More on how that might happen later.
  #
  # If a weird and rare case like this is observed, rsync will exit with an
  # error when attempting to update the times on these symbolic links.  rsync
  # may not be intelligent enough to try creating a new symbolic link in these
  # cases, but this script can be.
  #
  # This fix-up is not necessary when running as root, because root will
  # always be able to write everything needed.
  #
  # The problem occurs when an administrative user first drag-installs the
  # application to /Applications, resulting in the program's user being set to
  # the user's own ID.  If, subsequently, a .pkg package is installed over
  # that, the existing directory ownership will be preserved, but file
  # ownership will be changed to whateer is specified by the package,
  # typically root.  This applies to symbolic links as well.  On a subsequent
  # update, rsync will be able to copy the new files into place, because the
  # user still has permission to write to the directories.  If the symbolic
  # link targets are not changing, though, rsync will not replace them, and
  # they will remain owned by root.  The user will not have permission to
  # update the time on the symbolic links, resulting in an rsync error.
  if [[ ${EUID} -ne 0 ]]; then
    # This step isn't critical.
    set +e
    note "fixing installed symbolic links"

    # Only consider symbolic links in ${update_app}.  If there are any other
    # links in ${installed_app} not present in ${update_app}, rsync will
    # delete them as needed later.  Use find -print0 with read -d $'\0' to
    # handle even the weirdest paths.
    local update_link
    while IFS= read -r -d $'\0' update_link; do
      # ${update_link} is relative to ${update_app}.  Prepending
      # ${installed_app} looks for the same link already on disk.
      local installed_link="${installed_app}/${update_link}"
      note "ensure_writable_symlink ${installed_link}"
      ensure_writable_symlink "${installed_link}"
    done < <(cd "${update_app}" && find . -type l -print0)

    # Go back to how things were.
    set -e
  fi

  # By copying to ${installed_app}, the existing application name will be
  # preserved, if the user has renamed the application on disk.  Respecting
  # the user's changes is friendly.

  # Make sure that ${installed_versions_dir} exists, so that it can receive
  # the versioned directory.  It may not exist if updating from an older
  # version that did not use the versioned layout on disk.  An rsync that
  # excludes all contents is used to bring the permissions over from
  # ${update_versions_dir}, otherwise, this directory would be the only one in
  # the entire update exempt from getting its permissions copied over.  A
  # simple mkdir wouldn't copy mode bits.  This is done even if
  # ${installed_versions_dir} already does exist to ensure that the mode bits
  # come from the update.
  #
  # ${installed_app} is guaranteed to exist at this point, but
  # ${installed_app}/${CONTENTS_DIR} may not if things are severely broken or
  # if this update is actually an initial installation from a Keystone
  # skeleton bootstrap.  The mkdir creates ${installed_app}/${CONTENTS_DIR} if
  # it doesn't exist; its mode bits will be fixed up in a subsequent rsync.
  note "creating CONTENTS_DIR"
  if ! mkdir -p "${installed_app}/${CONTENTS_DIR}"; then
    err "mkdir of CONTENTS_DIR failed"
    exit 5
  fi

  local update_versions_dir="${update_app}/${VERSIONS_DIR}"
  note "update_versions_dir = ${update_versions_dir}"

  note "rsyncing VERSIONS_DIR"
  if ! rsync ${RSYNC_FLAGS} --exclude "*" "${update_versions_dir}/" \
                                          "${installed_versions_dir}"; then
    err "rsync of VERSIONS_DIR failed, status ${PIPESTATUS[0]}"
    exit 6
  fi

  # Copy the versioned directory.  The new versioned directory should have a
  # different name than any existing one, so this won't harm anything already
  # present in ${installed_versions_dir}, including the versioned directory
  # being used by any running processes.  If this step is interrupted, there
  # will be an incomplete versioned directory left behind, but it won't
  # won't interfere with anything, and it will be replaced or removed during a
  # future update attempt.  Note that in certain cases, same-version updates
  # are distributed to move users between channels; when this happens, the
  # contents of the versioned directories are identical and rsync will not
  # render the versioned directory unusable even for an instant.
  local update_versioned_dir new_versioned_dir
  update_versioned_dir="${update_versions_dir}/${update_version_app}"
  note "update_versioned_dir = ${update_versioned_dir}"
  new_versioned_dir="${installed_versions_dir}/${update_version_app}"
  note "new_versioned_dir = ${new_versioned_dir}"

  note "rsyncing versioned directory"
  if ! rsync ${RSYNC_FLAGS} --delete-before "${update_versioned_dir}/" \
                                            "${new_versioned_dir}"; then
    err "rsync of versioned directory failed, status ${PIPESTATUS[0]}"
    exit 7
  fi

  # Copy the unversioned files into place, leaving everything in
  # ${installed_versions_dir} alone.  If this step is interrupted, the
  # application will at least remain in a usable state, although it may not
  # pass signature validation.  Depending on when this step is interrupted,
  # the application will either launch the old or the new version.  The
  # critical point is when the main executable is replaced.  There isn't very
  # much to copy in this step, because most of the application is in the
  # versioned directory.  This step only accounts for around 50 files, most of
  # which are small localized InfoPlist.strings files.
  note "rsyncing app directory"
  if ! rsync ${RSYNC_FLAGS} --delete-after --exclude "/${VERSIONS_DIR}" \
       "${update_app}/" "${installed_app}"; then
    err "rsync of app directory failed, status ${PIPESTATUS[0]}"
    exit 8
  fi

  note "rsyncs complete"

  # If necessary, touch the outermost .app so that it appears to the outside
  # world that something was done to the bundle.  This will cause
  # LaunchServices to invalidate the information it has cached about the
  # bundle even if lsregister does not run.  This is not done if rsync already
  # updated the timestamp to something newer than what had been on disk.  This
  # is not considered a critical step, and if it fails, this script will not
  # exit.
  if [[ -n "${needs_touch}" ]]; then
    touch -cf "${installed_app}" || true
  fi

  # Read the new values, such as the version.
  note "reading new values"

  local new_version_app
  if ! new_version_app="$(defaults read "${installed_app_plist}" \
                                        "${APP_VERSION_KEY}")" ||
     [[ -z "${new_version_app}" ]]; then
    err "couldn't determine new_version_app"
    exit 9
  fi
  note "new_version_app = ${new_version_app}"

  local new_versioned_dir="${installed_versions_dir}/${new_version_app}"
  note "new_versioned_dir = ${new_versioned_dir}"

  local new_ks_plist="${installed_app_plist}"
  note "new_ks_plist = ${new_ks_plist}"

  local new_version_ks
  if ! new_version_ks="$(defaults read "${new_ks_plist}" \
                                       "${KS_VERSION_KEY}")" ||
     [[ -z "${new_version_ks}" ]]; then
    err "couldn't determine new_version_ks"
    exit 9
  fi
  note "new_version_ks = ${new_version_ks}"

  local update_url
  if ! update_url="$(defaults read "${new_ks_plist}" "${KS_URL_KEY}")" ||
     [[ -z "${update_url}" ]]; then
    err "couldn't determine update_url"
    exit 9
  fi
  note "update_url = ${update_url}"

  # The channel ID is optional.  Suppress stderr to prevent Keystone from
  # seeing possible error output.
  local channel
  channel="$(defaults read "${new_ks_plist}" "${KS_CHANNEL_KEY}" 2> /dev/null ||
             true)"
  note "channel = ${channel}"

  # Make sure that the update was successful by comparing the version found in
  # the update with the version now on disk.
  if [[ "${new_version_ks}" != "${update_version_ks}" ]]; then
    err "new_version_ks and update_version_ks do not match"
    exit 10
  fi

  # Notify LaunchServices.  This is not considered a critical step, and
  # lsregister's exit codes shouldn't be confused with this script's own.
  # Redirect stdout to /dev/null to suppress the useless "ThrottleProcessIO:
  # throttling disk i/o" messages that lsregister might print.
  note "notifying LaunchServices"
  local cs_fwk="/System/Library/Frameworks/CoreServices.framework"
  local ls_fwk="${cs_fwk}/Frameworks/LaunchServices.framework"
  local lsregister="${ls_fwk}/Support/lsregister"
  note "cs_fwk = ${cs_fwk}"
  note "ls_fwk = ${ls_fwk}"
  note "lsregister = ${lsregister}"
  "${lsregister}" "${installed_app}" > /dev/null || true

  # The brand information is stored differently depending on whether this is
  # running for a system or user ticket.
  note "handling brand code"

  local set_brand_file_access=
  local brand_plist
  if [[ -n "${system_ticket}" ]]; then
    # System ticket.
    set_brand_file_access="y"
    brand_plist="/${UNROOTED_BRAND_PLIST}"
  else
    # User ticket.
    brand_plist=~/"${UNROOTED_BRAND_PLIST}"
  fi
  local brand_plist_path="${brand_plist}.plist"
  note "set_brand_file_access = ${set_brand_file_access}"
  note "brand_plist = ${brand_plist}"
  note "brand_plist_path = ${brand_plist_path}"

  # If the user manually updated their copy of Chrome, there might be new
  # brand information in the app bundle, and that needs to be copied out into
  # the file Keystone looks at.
  if [[ -n "${old_brand}" ]]; then
    local brand_dir
    brand_dir="$(dirname "${brand_plist_path}")"
    note "brand_dir = ${brand_dir}"
    if ! mkdir -p "${brand_dir}"; then
      err "couldn't mkdir brand_dir, continuing"
    else
      if ! defaults write "${brand_plist}" "${KS_BRAND_KEY}" \
                          -string "${old_brand}"; then
        err "couldn't write brand_plist, continuing"
      elif [[ -n "${set_brand_file_access}" ]]; then
        if ! chown "root:wheel" "${brand_plist_path}"; then
          err "couldn't chown brand_plist_path, continuing"
        else
          if ! chmod 644 "${brand_plist_path}"; then
            err "couldn't chmod brand_plist_path, continuing"
          fi
        fi
      fi
    fi
  fi

  # Confirm that the brand file exists.  It's optional.
  local ksadmin_brand_plist_path="${brand_plist_path}"
  local ksadmin_brand_key="${KS_BRAND_KEY}"
  if [[ ! -f "${ksadmin_brand_plist_path}" ]]; then
    # Clear any branding information.
    ksadmin_brand_plist_path=
    ksadmin_brand_key=
  fi
  note "ksadmin_brand_plist_path = ${ksadmin_brand_plist_path}"
  note "ksadmin_brand_key = ${ksadmin_brand_key}"

  note "notifying Keystone"

  local ksadmin_args=(
    --register
    -P "${product_id}"
    --version "${new_version_ks}"
    --xcpath "${installed_app}"
    --url "${update_url}"
  )

  if ksadmin_supports_tag; then
    ksadmin_args+=(
      --tag "${channel}"
    )
  fi

  if ksadmin_supports_tagpath_tagkey; then
    ksadmin_args+=(
      --tag-path "${installed_app_plist_path}"
      --tag-key "${KS_CHANNEL_KEY}"
    )
  fi

  if ksadmin_supports_brandpath_brandkey; then
    ksadmin_args+=(
      --brand-path "${ksadmin_brand_plist_path}"
      --brand-key "${ksadmin_brand_key}"
    )
  fi

  note "ksadmin_args = ${ksadmin_args[*]}"

  if ! ksadmin "${ksadmin_args[@]}"; then
    err "ksadmin failed"
    exit 11
  fi

  # The remaining steps are not considered critical.
  set +e

  # Try to clean up old versions that are not in use.  The strategy is to keep
  # the versioned directory corresponding to the update just applied
  # (obviously) and the version that was just replaced, and to use ps and lsof
  # to see if it looks like any processes are currently using any other old
  # directories.  Directories not in use are removed.  Old versioned
  # directories that are in use are left alone so as to not interfere with
  # running processes.  These directories can be cleaned up by this script on
  # future updates.
  #
  # To determine which directories are in use, both ps and lsof are used.
  # Each approach has limitations.
  #
  # The ps check looks for processes within the verisoned directory.  Only
  # helper processes, such as renderers, are within the versioned directory.
  # Browser processes are not, so the ps check will not find them, and will
  # assume that a versioned directory is not in use if a browser is open
  # without any windows.  The ps mechanism can also only detect processes
  # running on the system that is performing the update.  If network shares
  # are involved, all bets are off.
  #
  # The lsof check looks to see what processes have the framework dylib open.
  # Browser processes will have their versioned framework dylib open, so this
  # check is able to catch browsers even if there are no associated helper
  # processes.  Like the ps check, the lsof check is limited to processes on
  # the system that is performing the update.  Finally, unless running as
  # root, the lsof check can only find processes running as the effective user
  # performing the update.
  #
  # These limitations are motiviations to additionally preserve the versioned
  # directory corresponding to the version that was just replaced.
  note "cleaning up old versioned directories"

  local versioned_dir
  for versioned_dir in "${installed_versions_dir}/"*; do
    note "versioned_dir = ${versioned_dir}"
    if [[ "${versioned_dir}" = "${new_versioned_dir}" ]] || \
       [[ "${versioned_dir}" = "${old_versioned_dir}" ]]; then
      # This is the versioned directory corresponding to the update that was
      # just applied or the version that was previously in use.  Leave it
      # alone.
      note "versioned_dir is new_versioned_dir or old_versioned_dir, skipping"
      continue
    fi

    # Look for any processes whose executables are within this versioned
    # directory.  They'll be helper processes, such as renderers.  Their
    # existence indicates that this versioned directory is currently in use.
    local ps_string="${versioned_dir}/"
    note "ps_string = ${ps_string}"

    # Look for any processes using the framework dylib.  This will catch
    # browser processes where the ps check will not, but it is limited to
    # processes running as the effective user.
    local lsof_file="${versioned_dir}/${FRAMEWORK_DIR}/${FRAMEWORK_NAME}"
    note "lsof_file = ${lsof_file}"

    # ps -e displays all users' processes, -ww causes ps to not truncate
    # lines, -o comm instructs it to only print the command name, and the =
    # tells it to not print a header line.
    # The cut invocation filters the ps output to only have at most the number
    # of characters in ${ps_string}.  This is done so that grep can look for
    # an exact match.
    # grep -F tells grep to look for lines that are exact matches (not regular
    # expressions), -q tells it to not print any output and just indicate
    # matches by exit status, and -x tells it that the entire line must match
    # ${ps_string} exactly, as opposed to matching a substring.  A match
    # causes grep to exit zero (true).
    #
    # lsof will exit nonzero if ${lsof_file} does not exist or is open by any
    # process.  If the file exists and is open, it will exit zero (true).
    if (! ps -ewwo comm= | \
          cut -c "1-${#ps_string}" | \
          grep -Fqx "${ps_string}") &&
       (! lsof "${lsof_file}" >& /dev/null); then
      # It doesn't look like anything is using this versioned directory.  Get
      # rid of it.
      note "versioned_dir doesn't appear to be in use, removing"
      rm -rf "${versioned_dir}"
    else
      note "versioned_dir is in use, skipping"
    fi
  done

  # If this script is being driven by a user Keystone ticket, it is not
  # running as root.  If the application is installed somewhere under
  # /Applications, try to make it writable by all admin users.  This will
  # allow other admin users to update the application from their own user
  # Keystone instances.
  #
  # If the script is being driven by a user Keystone ticket (not running as
  # root) and the application is not installed under /Applications, it might
  # not be in a system-wide location, and it probably won't be something that
  # other users on the system are running, so err on the side of safety and
  # don't make it group-writable.
  #
  # If this script is being driven by a system ticket (running as root), it's
  # future updates can be expected to be applied the same way, so admin-
  # writability is not a concern.  Set the entire thing to be owned by root
  # in that case, regardless of where it's installed, and drop any group and
  # other write permission.
  #
  # If this script is running as a user that is not a member of the admin
  # group, the chgrp operation will not succeed.  Tolerate that case, because
  # it's better than the alternative, which is to make the application
  # world-writable.
  note "setting permissions"

  local chmod_mode="a+rX,u+w,go-w"
  if [[ -z "${system_ticket}" ]]; then
    if [[ "${installed_app:0:14}" = "/Applications/" ]] &&
       chgrp -Rh admin "${installed_app}" 2> /dev/null; then
      chmod_mode="a+rX,ug+w,o-w"
    fi
  else
    chown -Rh root:wheel "${installed_app}" 2> /dev/null
  fi

  note "chmod_mode = ${chmod_mode}"
  chmod -R "${chmod_mode}" "${installed_app}" 2> /dev/null

  # On the Mac, or at least on HFS+, symbolic link permissions are significant,
  # but chmod -R and -h can't be used together.  Do another pass to fix the
  # permissions on any symbolic links.
  find "${installed_app}" -type l -exec chmod -h "${chmod_mode}" {} + \
      2> /dev/null

  # Host OS version check, to be able to take advantage of features on newer
  # systems and fall back to slow ways of doing things on older systems.
  local os_version
  os_version="$(sw_vers -productVersion)"
  note "os_version = ${os_version}"

  local os_major=0
  local os_minor=0
  if [[ "${os_version}" =~ ^([0-9]+)\.([0-9]+) ]]; then
    os_major="${BASH_REMATCH[1]}"
    os_minor="${BASH_REMATCH[2]}"
  fi
  note "os_major = ${os_major}"
  note "os_minor = ${os_minor}"

  # If an update is triggered from within the application itself, the update
  # process inherits the quarantine bit (LSFileQuarantineEnabled).  Any files
  # or directories created during the update will be quarantined in that case,
  # which may cause Launch Services to display quarantine UI.  That's bad,
  # especially if it happens when the outer .app launches a quarantined inner
  # helper.  If the application is already on the system and is being updated,
  # then it can be assumed that it should not be quarantined.  Use xattr to
  # drop the quarantine attribute.
  #
  # TODO(mark): Instead of letting the quarantine attribute be set and then
  # dropping it here, figure out a way to get the update process to run
  # without LSFileQuarantineEnabled even when triggering an update from within
  # the application.
  note "lifting quarantine"

  if [[ ${os_major} -gt 10 ]] ||
     ([[ ${os_major} -eq 10 ]] && [[ ${os_minor} -ge 6 ]]); then
    # On 10.6, xattr supports -r for recursive operation.
    xattr -d -r "${QUARANTINE_ATTR}" "${installed_app}" 2> /dev/null
  else
    # On earlier systems, xattr doesn't support -r, so run xattr via find.
    find "${installed_app}" -exec xattr -d "${QUARANTINE_ATTR}" {} + \
        2> /dev/null
  fi

  # Great success!
  note "done!"
  return 0
}

# Check "less than" instead of "not equal to" in case Keystone ever changes to
# pass more arguments.
if [[ ${#} -lt 1 ]]; then
  usage
  exit 2
fi

main "${@}"
exit ${?}
