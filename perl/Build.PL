#!/usr/bin/perl
#

use Module::Build;
use strict;


# Create the Build script
my $build = Module::Build->new
(
	module_name => 'Audio::JackMiniMix',
	license => 'gpl',
	build_requires => {
		'Module::Build' => '0.20'
	},
	
	create_makefile_pl => 'passthrough',
	
	requires => {
		'Test' => '1.00',
		'Net::LibLO' => '0.03',
	},
);

$build->create_build_script;
