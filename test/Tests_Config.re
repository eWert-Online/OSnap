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
        snapshotDirectory: "./__snapshots__",
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
