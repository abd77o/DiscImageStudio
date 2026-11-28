# If it does not burn

1. **Run as Administrator** (the exe asks for it; a "access denied" message in the status bar means it was not elevated).
2. **Test write does not burn anything.** It only simulates. After one successful test write the *Test write*
   switch turns off by itself; then press **Burn disc**.
3. **"This drive does not support test write"** (common on newer drives): press **Unlock real burn** in the bottom bar
   (or Advanced -> *Skip the test-write requirement*) and burn a spare blank CD-R.
4. Use a **blank CD-R** (not DVD, not a used disc) and a lower speed (4x or 8x) first.
5. Press **Open burn log** (appears after an error; also in Advanced). `burn_log.txt` next to the exe lists every
   command sent to the drive and the drive's decoded answer, e.g.

       +   120ms  MODE SELECT(10)  ...  -> FAILED status=02 : Illegal request: invalid field in parameter list (05/26/00)

   Attach this file when reporting a problem.

Typical messages: `Not ready: medium not present (02/3A/00)` = no disc; `Illegal request: illegal mode for this
track (05/64/00)` or `invalid field in CDB (05/24/00)` = the drive rejected the raw-DAO setup (tell us your drive model);
`Medium error: write error (03/0C/00)` = bad disc or speed too high.
