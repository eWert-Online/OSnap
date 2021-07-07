let log = (~header, message) => {
  Logs.app(m => m(~header, "%a", Fmt.styled(`Faint, Fmt.string), message));
};
