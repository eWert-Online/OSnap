let test_name = (~name, ~width, ~height) => {
  let width = Int.to_string(width);
  let height = Int.to_string(height);
  let padding_length = 8 - String.length(width) - String.length(height);
  let padding = padding_length > 0 ? String.make(padding_length, ' ') : "";
  <Pastel>
    name
    " "
    <Pastel dim=true> "(" width "x" height ")" padding </Pastel>
    "\t"
  </Pastel>;
};

let created_message = (~name, ~width, ~height) => {
  Console.log(
    <Pastel>
      <Pastel color=Blue bold=true> "CREATE" </Pastel>
      "\t"
      {test_name(~name, ~width, ~height)}
    </Pastel>,
  );
};

let skipped_message = (~name, ~width, ~height) => {
  Console.log(
    <Pastel>
      <Pastel color=Yellow bold=true> "SKIP" </Pastel>
      "\t"
      {test_name(~name, ~width, ~height)}
    </Pastel>,
  );
};

let success_message = (~name, ~width, ~height) => {
  Console.log(
    <Pastel>
      <Pastel color=Green bold=true> "PASS" </Pastel>
      "\t"
      {test_name(~name, ~width, ~height)}
    </Pastel>,
  );
};

let layout_message = (~name, ~width, ~height) => {
  Console.log(
    <Pastel>
      <Pastel color=Red bold=true> "FAIL" </Pastel>
      "\t"
      {test_name(~name, ~width, ~height)}
      <Pastel color=Red> "Images have different layout." </Pastel>
    </Pastel>,
  );
};

let diff_message = (~name, ~width, ~height, ~diffCount, ~diffPercentage) => {
  Console.log(
    <Pastel>
      <Pastel color=Red bold=true> "FAIL" </Pastel>
      "\t"
      {test_name(~name, ~width, ~height)}
      <Pastel color=Red>
        "Different pixels: "
        {Printf.sprintf("%i (%f%%)", diffCount, diffPercentage)}
      </Pastel>
    </Pastel>,
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
  Console.log(
    <Pastel>
      "\n"
      "\n"
      "Done! 🚀\n"
      "I did run a total of "
      <Pastel bold=true> {Int.to_string(test_count)} " snapshots" </Pastel>
      " in "
      <Pastel bold=true>
        {Printf.sprintf("%.3f", seconds)}
        " seconds"
      </Pastel>
      "!"
      "\n\nResults:\n"
      {create_count > 0
         ? <Pastel bold=true>
             {Int.to_string(create_count)}
             " Snapshots created \n"
           </Pastel>
         : ""}
      {skipped_count > 0
         ? <Pastel color=Yellow bold=true>
             {Int.to_string(skipped_count)}
             " Snapshots skipped \n"
           </Pastel>
         : ""}
      <Pastel color=Green bold=true>
        {Int.to_string(passed_count)}
        " Snapshots passed \n"
      </Pastel>
      {failed_count > 0
         ? <Pastel color=Red bold=true>
             {Int.to_string(failed_count)}
             " Snapshots failed \n"
           </Pastel>
         : ""}
    </Pastel>,
  );
};
