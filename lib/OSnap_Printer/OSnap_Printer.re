open Fmt;

let test_name = (~name, ~width, ~height) => {
  Fmt.str_like(
    Fmt.stdout,
    "%s %a",
    name,
    styled(`Faint, string),
    Printf.sprintf("(%ix%i)", width, height),
  );
};

let created_message = (~name, ~width, ~height) => {
  Fmt.pr(
    "%a\t%s @.",
    styled(`Bold, styled(`Blue, string)),
    "CREATE",
    test_name(~name, ~width, ~height),
  );
};

let skipped_message = (~name, ~width, ~height) => {
  Fmt.pr(
    "%a\t%s @.",
    styled(`Bold, styled(`Yellow, string)),
    "SKIP",
    test_name(~name, ~width, ~height),
  );
};

let success_message = (~name, ~width, ~height) => {
  Fmt.pr(
    "%a\t%s @.",
    styled(`Bold, styled(`Green, string)),
    "PASS",
    test_name(~name, ~width, ~height),
  );
};

let layout_message = (~name, ~width, ~height) => {
  Fmt.pr(
    "%a\t%s %a @.",
    styled(`Bold, styled(`Red, string)),
    "FAIL",
    test_name(~name, ~width, ~height),
    styled(`Red, string),
    "Images have different layout.",
  );
};

let diff_message = (~name, ~width, ~height, ~diffCount, ~diffPercentage) => {
  Fmt.pr(
    "%a\t%s %a @.",
    styled(`Bold, styled(`Red, string)),
    "FAIL",
    test_name(~name, ~width, ~height),
    styled(`Red, string),
    Printf.sprintf("Different pixels: %i (%f%%)", diffCount, diffPercentage),
  );
};

let stats =
    (
      ~test_count,
      ~create_count,
      ~passed_count,
      ~failed_count,
      ~skipped_count,
      ~seconds,
    ) => {
  let (%) = mod_float;

  let hours = {
    let t = Int.of_float(seconds /. 3600.);
    if (t > 0) {
      Fmt.str_like(
        Fmt.stdout,
        t == 1 ? "%a hour, " : "%a hours, ",
        styled(`Bold, int),
        t,
      );
    } else {
      "";
    };
  };

  let minutes = {
    let t = Int.of_float(seconds % 3600. /. 60.);
    if (t > 0 || hours != "") {
      Fmt.str_like(
        Fmt.stdout,
        t == 1 ? "%a minute and " : "%a minutes and ",
        styled(`Bold, int),
        t,
      );
    } else {
      "";
    };
  };

  let seconds = {
    let t = seconds % 3600. % 60.;
    Fmt.str_like(
      Fmt.stdout,
      "%a seconds",
      styled(`Bold, float_dfrac(minutes == "" ? 3 : 0)),
      t,
    );
  };

  Fmt.pr(
    "\n\nDone! 🚀\nI did run a total of %a snapshots in %s%s%s \n@.",
    styled(`Bold, int),
    test_count,
    hours,
    minutes,
    seconds,
  );
  Fmt.pr("Results:@.");

  if (create_count > 0) {
    Fmt.pr(
      "%a %a @.",
      styled(`Bold, int),
      create_count,
      styled(`Bold, string),
      "Snapshots created",
    );
  };

  if (skipped_count > 0) {
    Fmt.pr(
      "%a %a @.",
      styled(`Bold, styled(`Yellow, int)),
      skipped_count,
      styled(`Bold, styled(`Yellow, string)),
      "Snapshots skipped",
    );
  };

  Fmt.pr(
    "%a %a @.",
    styled(`Bold, styled(`Green, int)),
    passed_count,
    styled(`Bold, styled(`Green, string)),
    "Snapshots passed",
  );

  if (failed_count > 0) {
    Fmt.pr(
      "%a %a @.",
      styled(`Bold, styled(`Red, int)),
      failed_count,
      styled(`Bold, styled(`Red, string)),
      "Snapshots failed",
    );
  };
};
