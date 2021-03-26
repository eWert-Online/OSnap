open OSnap_CDP;
open OSnap_Browser_Target;

let wait_for = (~event, target) => {
  let sessionId = target.sessionId;
  let (p, resolver) = Lwt.wait();
  let callback = (_, remove) => {
    remove();
    Lwt.wakeup_later(resolver, ());
  };
  OSnap_Websocket.listen(~event, ~sessionId, callback);
  p;
};

let wait_for_network_idle = (target, ~loaderId) => {
  let sessionId = target.sessionId;
  let (p, resolver) = Lwt.wait();

  OSnap_Websocket.listen(
    ~event=Page.Events.LifecycleEvent.name,
    ~sessionId,
    (response, remove) => {
      open Page.Events;
      let eventData = LifecycleEvent.parse(response);
      let name = eventData.Types.Event.params.LifecycleEvent.name;
      let returned_loaderId =
        eventData.Types.Event.params.LifecycleEvent.loaderId;
      if (name == "networkIdle" && loaderId == returned_loaderId) {
        remove();
        Lwt.wakeup_later(resolver, ());
      };
    },
  );

  p;
};

let go_to = (~url, target) => {
  let sessionId = target.sessionId;
  let%lwt payload =
    Page.Navigate.make(url, ~sessionId)
    |> OSnap_Websocket.send
    |> Lwt.map(Page.Navigate.parse);

  payload.Types.Response.result.Page.Navigate.loaderId |> Lwt.return;
};

// let run_action: (action, t) => Lwt_result.t(t, string) =
//   (_a, _b) => {
//     Lwt_result.return(Obj.magic());
//   };

let set_size = (~width, ~height, target) => {
  let sessionId = target.sessionId;

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
  let sessionId = target.sessionId;

  let%lwt clip =
    if (full_size) {
      let%lwt metrics =
        Page.GetLayoutMetrics.make(~sessionId, ())
        |> OSnap_Websocket.send
        |> Lwt.map(Page.GetLayoutMetrics.parse)
        |> Lwt.map(response => response.Types.Response.result);
      let width = metrics.contentSize.width;
      let height = metrics.contentSize.height;

      Lwt.return(
        Some(Types.Page.Viewport.{x: 0, y: 0, width, height, scale: 1}),
      );
    } else {
      Lwt.return(None);
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
