# StreamCore SDK Demo iOS Resources

This directory contains bundle resources required by the iOS demo target.

- `streamcore_demo.lic`: encrypted `SC-LIC-ENC-v1` demo license bound to package
  identifier `com.hbr.streamcoredemo`.

`StreamCoreDemoViewController` loads these resources from `NSBundle.mainBundle`.
The Demo SDK already embeds its verification key and decryption profile; the app
must not provide or override them.
If either file is missing from the app bundle, the License page will report the
runtime configuration error.
