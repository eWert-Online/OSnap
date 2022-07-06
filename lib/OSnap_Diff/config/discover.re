module C = Configurator.V1;

C.main(~name="odiff-c-lib-package-resolver", _c => {
  let spng_include_path = Sys.getenv("SPNG_INCLUDE_PATH") |> String.trim;
  let spng_lib_path = Sys.getenv("SPNG_LIB_PATH") |> String.trim;
  let libspng = spng_lib_path ++ "/libspng_static.a";

  let z_lib_path = Sys.getenv("Z_LIB_PATH") |> String.trim;
  let zlib = z_lib_path ++ "/libz.a";

  C.Flags.write_sexp(
    "png_write_c_flags.sexp",
    ["-fPIC", "-I" ++ spng_include_path],
  );
  C.Flags.write_sexp("png_write_c_library_flags.sexp", [libspng, zlib]);
  C.Flags.write_sexp("png_write_flags.sexp", ["-cclib", libspng]);

  C.Flags.write_sexp(
    "png_c_flags.sexp",
    ["-fPIC", "-I" ++ spng_include_path],
  );
  C.Flags.write_sexp("png_c_library_flags.sexp", [libspng, zlib]);
  C.Flags.write_sexp("png_flags.sexp", ["-cclib", libspng]);
});
