deps = {
  "src/breakpad/src":
    "http://google-breakpad.googlecode.com/svn/trunk/src@281",

  "src/googleurl":
    "http://google-url.googlecode.com/svn/trunk@92",

  "src/testing/gtest":
    "http://googletest.googlecode.com/svn/trunk@63",

  "src/third_party/WebKit":
    "/trunk/third_party/WebKit@19",

  "src/third_party/icu38":
    "/trunk/deps/third_party/icu38@1197",

  "src/v8":
    "https://svn/r/googleclient/v8/trunk@131883",

  "src/webkit/data/layout_tests/LayoutTests":
    "http://svn.webkit.org/repository/webkit/branches/Safari-3-1-branch/LayoutTests@31256",
}

include_rules = [
  # Everybody can use some things.
  "+base",
  "+build",

  # For now, we allow ICU to be included by specifying "unicode/...", although
  # this should probably change.
  "+unicode"
]
