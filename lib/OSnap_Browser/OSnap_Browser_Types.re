type t = {
  ws: string,
  targetId: OSnap_CDP.Types.Target.TargetID.t,
  sessionId: OSnap_CDP.Types.Target.SessionID.t,
  process: {
    .
    close: Lwt.t(Unix.process_status),
    kill: int => unit,
    pid: int,
    rusage: Lwt.t(Lwt_unix.resource_usage),
    state: Lwt_process.state,
    status: Lwt.t(Unix.process_status),
    stderr: Lwt_io.channel(Lwt_io.input),
    stdin: Lwt_io.channel(Lwt_io.output),
    stdout: Lwt_io.channel(Lwt_io.input),
    terminate: unit,
  },
};
