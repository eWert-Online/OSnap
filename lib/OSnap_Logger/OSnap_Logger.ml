let src = Logs.Src.create "osnap"

let init style_renderer level =
  Fmt_tty.setup_std_outputs ?style_renderer ();
  Logs.Src.set_level src level;
  Logs.set_reporter (Logs_fmt.reporter ())
;;

let debug ~header message =
  Logs.debug ~src (fun log -> log ~header "%a" (Fmt.styled `Faint Fmt.string) message)
;;

let info ~header message = Logs.info ~src (fun log -> log ~header "%a" Fmt.string message)

let warn ~header message =
  Logs.warn ~src (fun log -> log ~header "%a" (Fmt.styled `Yellow Fmt.string) message)
;;

let error ~header message =
  Logs.err ~src (fun log -> log ~header "%a" (Fmt.styled `Red Fmt.string) message)
;;
