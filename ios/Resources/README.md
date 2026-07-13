# StreamCore SDK Demo iOS Resources

This directory contains bundle resources required by the iOS demo target.

- `streamcore_demo.lic`: demo license bound to package identifier `com.hbr.streamcoredemo`.
- `streamcore_demo_public.pem`: public key paired with the demo license.

`StreamCoreDemoViewController` loads these resources from `NSBundle.mainBundle`.
If either file is missing from the app bundle, the License page will report the
runtime configuration error.
