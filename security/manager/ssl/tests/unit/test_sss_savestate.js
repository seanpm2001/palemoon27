/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

// The purpose of this test is to see that the site security service properly
// writes its state file.

const EXPECTED_ENTRIES = 6;
const EXPECTED_HSTS_COLUMNS = 3;
const EXPECTED_HPKP_COLUMNS = 4;
let gProfileDir = null;

const NON_ISSUED_KEY_HASH = "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA=";

// For reference, the format of the state file is a list of:
// <domain name> <expiration time in milliseconds>,<sts status>,<includeSubdomains>
// separated by newlines ('\n')

function checkStateWritten(aSubject, aTopic, aData) {
  equal(aData, SSS_STATE_FILE_NAME);

  let stateFile = gProfileDir.clone();
  stateFile.append(SSS_STATE_FILE_NAME);
  ok(stateFile.exists());
  let stateFileContents = readFile(stateFile);
  // the last line is removed because it's just a trailing newline
  let lines = stateFileContents.split('\n').slice(0, -1);
  equal(lines.length, EXPECTED_ENTRIES);
  let sites = {}; // a map of domain name -> [the entry in the state file]
  for (let line of lines) {
    let parts = line.split('\t');
    let host = parts[0];
    let score = parts[1];
    let lastAccessed = parts[2];
    let entry = parts[3].split(',');
    let expectedColumns = EXPECTED_HSTS_COLUMNS;
    if (host.indexOf("HPKP") != -1) {
      expectedColumns = EXPECTED_HPKP_COLUMNS;
    }
    equal(entry.length, expectedColumns);
    sites[host] = entry;
  }

  // We can receive multiple data-storage-written events. In particular, we
  // may receive one where DataStorage wrote out data before we were done
  // processing all of our headers. In this case, the data may not be
  // as we expect. We only care about the final one being correct, however,
  // so we return and wait for the next event if things aren't as we expect.
  // sites[url][1] corresponds to SecurityPropertySet (if 1) and
  //                              SecurityPropertyUnset (if 0)
  // sites[url][2] corresponds to includeSubdomains
  if (sites["bugzilla.mozilla.org:HSTS"][1] != 1) {
    return;
  }
  if (sites["bugzilla.mozilla.org:HSTS"][2] != 0) {
    return;
  }
  if (sites["a.example.com:HSTS"][1] != 1) {
    return;
  }
  if (sites["a.example.com:HSTS"][2] != 1) {
    return;
  }
  if (sites["b.example.com:HSTS"][1] != 1) {
    return;
  }
  if (sites["b.example.com:HSTS"][2] != 0) {
    return;
  }
  if (sites["c.c.example.com:HSTS"][1] != 1) {
    return;
  }
  if (sites["c.c.example.com:HSTS"][2] != 1) {
    return;
  }
  if (sites["d.example.com:HSTS"][1] != 1) {
    return;
  }
  if (sites["d.example.com:HSTS"][2] != 0) {
    return;
  }
  if (sites["dynamic-pin.example.com:HPKP"][1] != 1) {
    return;
  }
  if (sites["dynamic-pin.example.com:HPKP"][2] != 1) {
    return;
  }
  equal(sites["dynamic-pin.example.com:HPKP"][3], NON_ISSUED_KEY_HASH);

  do_test_finished();
}

function run_test() {
  Services.prefs.setIntPref("test.datastorage.write_timer_ms", 100);
  gProfileDir = do_get_profile();
  let SSService = Cc["@mozilla.org/ssservice;1"]
                    .getService(Ci.nsISiteSecurityService);
  // Put an HPKP entry
  SSService.setKeyPins("dynamic-pin.example.com", true, 1000, 1,
                       [NON_ISSUED_KEY_HASH]);

  let uris = [ Services.io.newURI("http://bugzilla.mozilla.org", null, null),
               Services.io.newURI("http://a.example.com", null, null),
               Services.io.newURI("http://b.example.com", null, null),
               Services.io.newURI("http://c.c.example.com", null, null),
               Services.io.newURI("http://d.example.com", null, null) ];

  for (let i = 0; i < 1000; i++) {
    let uriIndex = i % uris.length;
    // vary max-age
    let maxAge = "max-age=" + (i * 1000);
     // alternate setting includeSubdomains
    let includeSubdomains = (i % 2 == 0 ? "; includeSubdomains" : "");
    let sslStatus = new FakeSSLStatus();
    SSService.processHeader(Ci.nsISiteSecurityService.HEADER_HSTS,
                            uris[uriIndex], maxAge + includeSubdomains,
                            sslStatus, 0);
  }

  do_test_pending();
  Services.obs.addObserver(checkStateWritten, "data-storage-written", false);
}
