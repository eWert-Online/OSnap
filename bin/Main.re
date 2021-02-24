open Lwt_result.Infix;

Lwt_main.run(
  {
    OSnap.Browser.make()
    >|= (
      browser => {
        print_endline(browser.OSnap.Browser.ws);
        (browser.process)#terminate;
      }
    );
  },
);
