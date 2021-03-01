open TestFramework;

describe("OSnap.Config.parse", ({test, _}) => {
  test("parses correct conf", ({expect, _}) => {
    let ic = open_in("test/test_files/osnap.config.json");
    let file_length = in_channel_length(ic);
    let contents = really_input_string(ic, file_length);
    close_in(ic);

    let config = OSnap.Config.parse(contents);
    expect.result(config).toBeOk();
    expect.equal(
      OSnap.Config.{
        baseUrl: "",
        fullScreen: true,
        defaultSizes: [
          (1920, 1080),
          (1600, 900),
          (768, 1024),
          (320, 568),
        ],
        snapshotDirectory: "./__image-snapshots__",
      },
      Result.get_ok(config),
    );
  });
  test("fails gracefully on incorrect conf", ({expect, _}) => {
    let ic = open_in("test/test_files/osnap.config_fail.json");
    let file_length = in_channel_length(ic);
    let contents = really_input_string(ic, file_length);
    close_in(ic);

    let config = OSnap.Config.parse(contents);
    expect.result(config).toBeError();
  });
});

describe("OSnap.Config.find", ({test, _}) => {
  test("finds default config", ({expect, _}) => {
    let config =
      OSnap.Config.find()
      |> Result.map(TestFramework.Utils.make_path_relative);

    expect.result(config).toBeOk();
    expect.string(Result.get_ok(config)).toEqual("test/test_files");
  });

  test("finds custom config", ({expect, _}) => {
    let config =
      OSnap.Config.find(~config_name="osnap.config_fail.json", ())
      |> Result.map(TestFramework.Utils.make_path_relative);

    expect.result(config).toBeOk();
    expect.string(Result.get_ok(config)).toEqual("test/test_files");
  });

  test("finds config in custom path", ({expect, _}) => {
    let config =
      OSnap.Config.find(
        ~base_path="test/test_files/configs",
        ~config_name="osnap-custom.config.json",
        (),
      )
      |> Result.map(TestFramework.Utils.make_path_relative);

    expect.result(config).toBeOk();
    expect.string(Result.get_ok(config)).toEqual("test/test_files/configs");
  });

  test("fails gracefully when config file is not found", ({expect, _}) => {
    let config =
      OSnap.Config.find(~config_name="osnap-non-existant.config.json", ())
      |> Result.map(TestFramework.Utils.make_path_relative);

    expect.result(config).toBeError();
  });
});
