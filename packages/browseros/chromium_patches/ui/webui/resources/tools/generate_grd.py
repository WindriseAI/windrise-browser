diff --git a/ui/webui/resources/tools/generate_grd.py b/ui/webui/resources/tools/generate_grd.py
index 7ff7e7e99304817095fbea3d3a3d29ce71e2492c..9652872d48345d41440929f86a58a096dfe961ae 100644
--- a/ui/webui/resources/tools/generate_grd.py
+++ b/ui/webui/resources/tools/generate_grd.py
@@ -37,6 +37,14 @@
 #   input_files_base_dir:
 #     The base directory for the paths in |input_files|. |input_files| and
 #     |input_files_base_dir| must either both be provided or both be omitted.
+#
+#   ignore_missing_input_files:
+#     If set, files from |input_files| that do not exist under
+#     |input_files_base_dir| are skipped.
+#
+#   compress_brotli_files:
+#     Optional list of filenames (matching input_files or manifest files) whose
+#     <include> rows should carry compress="brotli".
 
 import argparse
 import json
@@ -67,18 +75,23 @@ GRD_BEGIN_TEMPLATE = (
 GRD_INCLUDE_TEMPLATE = (
   '      <include name="{name}" '
   'file="{file}" resource_path="{path}" '
-  'use_base_dir="false" type="{type}" />\n'
+  'use_base_dir="false" type="{type}"{compress_attr} />\n'
 )
 
 GRD_END_TEMPLATE = '    </includes>\n  </release>\n</grit>\n'
 
 GRDP_BEGIN_TEMPLATE = '<?xml version="1.0" encoding="UTF-8"?>\n<grit-part>\n'
 GRDP_END_TEMPLATE = '</grit-part>\n'
 
 
 # Generates an <include .... /> row for the given file.
 def _generate_include_row(
-  grd_prefix, filename, pathname, resource_path_rewrites, resource_path_prefix
+  grd_prefix,
+  filename,
+  pathname,
+  resource_path_rewrites,
+  resource_path_prefix,
+  compress_brotli_files=None,
 ):
   assert '\\' not in filename
   assert '\\' not in pathname
@@ -106,8 +119,18 @@ def _generate_include_row(
     resource_path = resource_path_prefix + '/' + resource_path
   assert '\\' not in resource_path
 
+  compress_attr = (
+    ' compress="brotli"'
+    if compress_brotli_files and filename in compress_brotli_files
+    else ''
+  )
+
   return GRD_INCLUDE_TEMPLATE.format(
-    file=pathname, path=resource_path, name=name, type=type
+    file=pathname,
+    path=resource_path,
+    name=name,
+    type=type,
+    compress_attr=compress_attr,
   )
 
 
@@ -122,6 +145,8 @@ def main(argv):
   parser.add_argument('--root-gen-dir', required=True)
   parser.add_argument('--input-files', nargs="*")
   parser.add_argument('--input-files-base-dir')
+  parser.add_argument('--ignore-missing-input-files', action='store_true')
+  parser.add_argument('--compress-brotli-files', nargs="*")
   parser.add_argument('--output-files-base-dir', default='grit')
   parser.add_argument('--grdp-files', nargs="*")
   parser.add_argument('--resource-path-rewrites', nargs="*")
@@ -153,6 +178,8 @@ def main(argv):
         [original, rewrite] = r.split("|")
         resource_path_rewrites[original] = rewrite
 
+    compress_brotli_files = set(args.compress_brotli_files or [])
+
     if args.input_files != None:  # noqa: E711
       assert args.input_files_base_dir
       args.input_files_base_dir = args.input_files_base_dir.replace('\\', '/')
@@ -176,13 +203,22 @@ def main(argv):
           + 'input_files_base_dir'
         )
 
+        if args.ignore_missing_input_files:
+          real_base_dir = os.path.join(_CWD, '..', '..',
+                                       args.input_files_base_dir)
+          if args.input_files_base_dir.startswith(args.root_gen_dir + '/'):
+            real_base_dir = os.path.join(_CWD, args.input_files_base_dir)
+          if not os.path.exists(os.path.join(real_base_dir, filename)):
+            continue
+
         filepath = os.path.join(base_dir, filename).replace('\\', '/')
         grd_file.write(
           _generate_include_row(
             args.grd_prefix,
             filename,
             filepath,
             resource_path_rewrites,
             args.resource_path_prefix,
+            compress_brotli_files,
           )
         )
@@ -201,6 +237,7 @@ def main(argv):
                 '${root_gen_dir}/' + rebased_path,
                 resource_path_rewrites,
                 args.resource_path_prefix,
+                compress_brotli_files,
               )
             )
 
