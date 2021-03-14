open OSnap_Browser_Types;
open OSnap_CDP;

let wait_for = event => {
  let (p, resolver) = Lwt.wait();
  let callback = () => {
    Lwt.wakeup_later(resolver, ());
  };
  OSnap_Websocket.listen(event, callback);
  p;
};

let go_to = (url, browser) => {
  let%lwt payload =
    Page.Navigate.make(url, ~sessionId=browser.sessionId)
    |> OSnap_Websocket.send
    |> Lwt.map(Page.Navigate.parse);
  payload.Types.Response.result.Page.Navigate.frameId |> Lwt.return;
};

// let run_action: (action, t) => Lwt_result.t(t, string) =
//   (_a, _b) => {
//     Lwt_result.return(Obj.magic());
//   };

let set_size = (~width, ~height, browser) => {
  let%lwt _ =
    Emulation.SetDeviceMetricsOverride.make(
      ~sessionId=browser.sessionId,
      ~width,
      ~height,
      ~scale=1,
      ~deviceScaleFactor=0,
      ~mobile=false,
      (),
    )
    |> OSnap_Websocket.send;

  Lwt.return();
};

let screenshot = (~full_size=false, browser) => {
  let%lwt _ =
    Target.ActivateTarget.make(browser.targetId, ~sessionId=browser.sessionId)
    |> OSnap_Websocket.send;

  let%lwt metrics =
    Page.GetLayoutMetrics.make(~sessionId=browser.sessionId, ())
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
      ~sessionId=browser.sessionId,
      ~clip?,
      ~captureBeyondViewport=true,
      (),
    )
    |> OSnap_Websocket.send
    |> Lwt.map(Page.CaptureScreenshot.parse);

  response.Types.Response.result.Page.CaptureScreenshot.data |> Lwt.return;
};
