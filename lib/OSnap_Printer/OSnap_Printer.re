open Fmt;

let flush_fmt = () => Fmt.pr("@.");

let test_name = (~name, ~width, ~height) => {
  Fmt.pr(
    "%s %a",
    name,
    styled(`Faint, string),
    Printf.sprintf("(%ix%i)", width, height),
  );
};

let created_message = (~name, ~width, ~height) => {
  Fmt.pr("%a\t", styled(`Bold, styled(`Blue, string)), "CREATE");
  test_name(~name, ~width, ~height);
  flush_fmt();
};

let skipped_message = (~name, ~width, ~height) => {
  Fmt.pr("%a\t", styled(`Bold, styled(`Yellow, string)), "SKIP");
  test_name(~name, ~width, ~height);
  flush_fmt();
};

let success_message = (~name, ~width, ~height) => {
  Fmt.pr("%a\t", styled(`Bold, styled(`Green, string)), "PASS");
  test_name(~name, ~width, ~height);
  flush_fmt();
};

let layout_message = (~name, ~width, ~height) => {
  Fmt.pr("%a\t", styled(`Bold, styled(`Red, string)), "FAIL");
  test_name(~name, ~width, ~height);
  Fmt.pr("%a", styled(`Red, string), "Images have different layout.");
  flush_fmt();
};

let diff_message = (~name, ~width, ~height, ~diffCount, ~diffPercentage) => {
  Fmt.pr("%a\t", styled(`Bold, styled(`Red, string)), "FAIL");
  test_name(~name, ~width, ~height);
  Fmt.pr(
    "%a",
    styled(`Red, string),
    Printf.sprintf("Different pixels: %i (%f%%)", diffCount, diffPercentage),
  );
  flush_fmt();
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
  Fmt.pr(
    "\n\nDone! 🚀\nI did run a total of %a snapshots in %a seconds! \n@.",
    styled(`Bold, int),
    test_count,
    styled(`Bold, float_dfrac(3)),
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
