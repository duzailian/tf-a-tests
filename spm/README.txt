This directory provides sample Secure Partitions:

-Cactus is the main Secure Partition test payload run at S-EL1 on top of
the S-EL2 reference firmware.

-Ivy is the main Secure Partition test payload run at S-EL0 on top of
the S-EL2 reference firmware.

Both Cactus and Ivy comply with the FF-A 1.1 specification and provide
sample ABI calls for setup and discovery, direct request/response messages,
and memory sharing interfaces.

-Cactus-MM is the (legacy) S-EL0 test payload complying with the MM communication
interface. It is run at S-EL0 on top of TF-A's SPM-MM implementation at EL3.
