
use strict;
use Test;


# use a BEGIN block so we print our plan before loading modules
BEGIN { plan tests => 1 }


# Check Audio::JackMiniMix loads ok
use Audio::JackMiniMix;
ok(1);



exit;
