#!/usr/bin/perl

# changelog: update the changelog using your favorite visual editor
#
# When creating a new changelog section, if either of the environment
# variables CVS_EMAIL or EMAIL is set, changelog will use this as the
# uploader's email address (with the former taking precedence), and if
# CVS_FULLNAME is set, it will use this as the uploader's full name.
# Otherwise, it will just copy the values from the previous changelog
# entry.
#
# Originally by Christoph Lameter <clameter@debian.org>
# Modified extensively by Julian Gilbey <jdg@debian.org>
# Now a changelog script for GNU projects
# Sean 'Shaleh' Perry <shaleh@debian.org>
#
# Copyright 1999 by Julian Gilbey 
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

use 5.003;  # We're using function prototypes
use Getopt::Std;
use File::Copy;
use Cwd;

# Predeclare functions
sub fatal($);

my $progname;
($progname=$0) =~ s|.*/||;

sub usage {
	print <<EOF
Usage: $progname [-h] [changelog entry]

Options:
  -h     Display this help message and exit
EOF
}

# Look for the changelog
if (! -e "ChangeLog")
{
	die("Cannot find ChangeLog anywhere!  " .
		"Are you in the source code tree?");
}

if ( -e "ChangeLog.dch" ) {
	die("The backup file ChangeLog.dch already exists --\n" .
						"please move it before trying again");
} else {
	$tmpchk=1;
}

sub BEGIN {
	# Initialise the variable
	$tmpchk=0;
}

sub END {
	unlink "ChangeLog.dch" or
		warn "Could not remove ChangeLog.dch"
			if $tmpchk;
}

open S, "ChangeLog" or fatal "Cannot open ChangeLog:" . " $!";
open O, ">ChangeLog.dch"
	or fatal "Cannot write to temporary file:" . " $!";

getopts('h')
	or die sprintf
		("Usage: %s [-h] [changelog entry]\n", $progname);
if ($opt_h) { usage(); exit(0); }

# Get a possible changelog entry from the command line
$SAVETEXT=$TEXT="@ARGV";

# Get the date
chomp($DATE=`822-date`);

$MAINTAINER = $ENV{'CVS_FULLNAME'} || $MAINTAINER;
if ($MAINTAINER eq '') {
	fatal "Could not figure out maintainer's name:" . " $!";
}
$EMAIL = $ENV{'CVS_EMAIL'} || $ENV{'EMAIL'} || $EMAIL;
if ($EMAIL eq '') {
	fatal "Could not figure out maintainer's email address:" . " $!";
}

print O "$MAINTAINER <$EMAIL>  $DATE\n\n";
if ($TEXT) { write O } else { print O "  * \n\n"; $line=3; }
# Rewind the current changelog to the beginning of the file
seek S,0,0 or fatal "Couldn't rewind ChangeLog:" . " $!";
# Copy the rest of the changelog file to new one
while (<S>) { print O; }
	
close S or fatal "Error closing ChangeLog:" . " $!";
close O or fatal "Error closing temporary ChangeLog:" . " $!";

# Now Run the Editor
if (! $SAVETEXT) {
	$EDITOR=$ENV{"EDITOR"} || $ENV{"VISUAL"} || '/usr/bin/editor';
	system("$EDITOR +$line ChangeLog.dch") / 256 == 0 or
		fatal "Error editing the ChangeLog:" . " $!";
}

copy("ChangeLog.dch","ChangeLog") or
	fatal "Couldn't replace ChangeLog with new ChangeLog:" . " $!";

# Format for standard ChangeLogs
format O =
  * ^<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
    $TEXT

 ~~ ^<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
    $TEXT
.

sub fatal($) {
    my ($pack,$file,$line);
    ($pack,$file,$line) = caller();
    (my $msg = sprintf("%s: fatal error at line %d:\n",
				$progname, $line) . "@_\n") =~ tr/\0//d;
	 $msg =~ s/\n\n$/\n/;
    die $msg;
}

