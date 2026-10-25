# Readiness diagnostic correction

The current F2 report says calls are blocked because physical media is never automatically certified, even when the live dial button is enabled. Correct the report without changing call authorization.

Use the existing authorize_dial presentation gate on the same observed snapshot and destination as the button. Add one boolean to CallReadiness for dial control availability; show it separately from the six existing homologation indicators. The summary describes checklist completion, never infers call blocking from physical media validation. Actual commands still perform their own checks. No addresses, usernames, credentials or device IDs enter the formatter.

Approaches: changing only the summary removes the false claim but omits actionable button status; adopting the actual presentation gate as a separate indicator provides both. Chosen by implementer under repeated requests to proceed. No new SIP or hardware claims. Version0.55.1 patch, packaged with unchanged production gates.
