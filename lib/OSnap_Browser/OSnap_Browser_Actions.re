open OSnap_CDP;

let wait_for = event => {
  // TODO: Wait on a specific target
  let (p, resolver) = Lwt.wait();
  let callback = () => {
    Lwt.wakeup_later(resolver, ());
  };
  OSnap_Websocket.listen(event, callback);
  p;
};

let go_to = (~url, target) => {
  let (_targetId, sessionId) = target;
  let%lwt payload =
    Page.Navigate.make(url, ~sessionId)
    |> OSnap_Websocket.send
    |> Lwt.map(Page.Navigate.parse);
  payload.Types.Response.result.Page.Navigate.frameId |> Lwt.return;
};

// let run_action: (action, t) => Lwt_result.t(t, string) =
//   (_a, _b) => {
//     Lwt_result.return(Obj.magic());
//   };

let set_size = (~width, ~height, target) => {
  let (_targetId, sessionId) = target;

  let%lwt _ =
    Emulation.SetDeviceMetricsOverride.make(
      ~sessionId,
      ~width,
      ~height,
      ~deviceScaleFactor=1,
      ~mobile=false,
      (),
    )
    |> OSnap_Websocket.send;

  Lwt.return();
};

let screenshot = (~full_size=false, target) => {
  let (targetId, sessionId) = target;
  let%lwt _ =
    Target.ActivateTarget.make(targetId, ~sessionId) |> OSnap_Websocket.send;

  let%lwt metrics =
    Page.GetLayoutMetrics.make(~sessionId, ())
    |> OSnap_Websocket.send
    |> Lwt.map(Page.GetLayoutMetrics.parse)
    |> Lwt.map(response => response.Types.Response.result);
  let width = metrics.contentSize.width;
  let height = metrics.contentSize.height;

  let clip =
    if (full_size) {
      Some(Types.Page.Viewport.{x: 0, y: 0, width, height, scale: 1});
    } else {
      None;
    };

  let%lwt response =
    Page.CaptureScreenshot.make(
      ~format="png",
      ~sessionId,
      ~clip?,
      ~captureBeyondViewport=true,
      (),
    )
    |> OSnap_Websocket.send
    |> Lwt.map(Page.CaptureScreenshot.parse);

  response.Types.Response.result.Page.CaptureScreenshot.data |> Lwt.return;
};
