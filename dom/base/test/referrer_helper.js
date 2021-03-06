/*
 * common functionality for iframe, anchor, and area referrer attribute tests
 */
const GET_RESULT = SJS + 'ACTION=get-test-results';
const RESET_STATE = SJS + 'ACTION=resetState';

SimpleTest.waitForExplicitFinish();
var advance = function() { tests.next(); };

/**
 * Listen for notifications from the child.
 * These are sent in case of error, or when the loads we await have completed.
 */
window.addEventListener("message", function(event) {
  if (event.data == "childLoadComplete") {
    // all loads happen, continue the test.
    advance();
  }
});

/**
 * helper to perform an XHR
 * to do checkIndividualResults and resetState
 */
function doXHR(aUrl, onSuccess, onFail) {
  // The server is at http[s]://example.com so we need cross-origin XHR.
  var xhr = new XMLHttpRequest({mozSystem: true});
  xhr.responseType = "json";
  xhr.onload = function () {
    onSuccess(xhr);
  };
  xhr.onerror = function () {
    onFail(xhr);
  };
  xhr.open('GET', "http" + aUrl, true);
  xhr.send(null);
}

/**
 * Grabs the results via XHR and passes to checker.
 */
function checkIndividualResults(aTestname, aExpectedReferrer, aName) {
  var onload = xhr => {
    var results = xhr.response;
    info(JSON.stringify(xhr.response));
    ok(aName in results, aName + " tests have to be performed.");
    is(results[aName].policy, aExpectedReferrer, aTestname + ' --- ' + results[aName].policy + ' (' + results[aName].referrer + ')');
    advance();
  };
  var onerror = xhr => {
    ok(false, "Can't get results from the counter server.");
    SimpleTest.finish();
  };
  doXHR(GET_RESULT, onload, onerror);
}

function resetState() {
  doXHR(RESET_STATE,
    advance,
    function(xhr) {
      ok(false, "error in reset state");
      SimpleTest.finish();
    });
}

/**
 * testing if anchor and area referrer attributes are honoured (1174913)
 */
var tests = (function*() {

  // enable referrer attribute
  yield SpecialPowers.pushPrefEnv({"set": [['network.http.enablePerElementReferrer', true]]}, advance);
  yield SpecialPowers.pushPrefEnv({"set": [['security.mixed_content.block_active_content', false]]}, advance);
  yield SpecialPowers.pushPermissions([{'type': 'systemXHR', 'allow': true, 'context': document}], advance);

  var iframe = document.getElementById("testframe");

  for (var j = 0; j < testCases.length; j++) {
    var actions = testCases[j].ACTION;
    var tests = testCases[j].TESTS;
    for (var k = 0; k < actions.length; k++) {
      var actionString = actions[k];
      for (var i = 0; i < tests.length; i++) {
        yield resetState();
        var searchParams = new URLSearchParams();
        searchParams.append("ACTION", actionString);
        searchParams.append("NAME", tests[i].NAME);
        for (var l of PARAMS) {
          if (tests[i][l]) {
            searchParams.append(l, tests[i][l]);
          }
        }
        var schemeFrom = tests[i].SCHEME_FROM || "http";
        yield iframe.src = schemeFrom + SJS + searchParams.toString();
        yield checkIndividualResults(tests[i].DESC, tests[i].RESULT, tests[i].NAME);
      };
    };
  };

  // complete.
  SimpleTest.finish();
})();
