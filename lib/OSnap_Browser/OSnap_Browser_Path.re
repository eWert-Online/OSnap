// https://omahaproxy.appspot.com/deps.json?version=99.0.4844.74
let get_revision = () => "1056772";
let get_folder_name = () => "osnap_chromium_" ++ get_revision();
let get_chromium_path = () => {
  switch (Sys.getenv_opt("HOME")) {
  | Some(home) when Sys.is_directory(home) =>
    Filename.concat(home, get_folder_name())
  | _ => Filename.concat(Filename.get_temp_dir_name(), get_folder_name())
  };
};
