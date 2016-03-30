
Next Steps
==========

+ The disktool badly abuses global variables. That should be fixed. Eventually certain sections
  of the disktool should be split off and moved into the shared folder where they can be shared
  with the driver. The first step in doing that will be to clean up the undisciplined use of
  globals in the current disktool.
