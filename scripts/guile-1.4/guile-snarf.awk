#  Copyright (C) 1999, 2000, 2007 Free Software Foundation, Inc.
# 
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3, or (at your option)
# any later version.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this software; see the file COPYING.  If not, write to
# the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
# Boston, MA 02110-1301 USA
#
# Written by Greg J. Badros, <gjb@cs.washington.edu>
# 12-Dec-1999

BEGIN { FS="|"; 
        dot_doc_file = ARGV[1]; ARGV[1] = "-";
        std_err = "/dev/stderr";
        # be sure to put something in the files to help make out
        print "";
        printf "" > dot_doc_file;
}

/^[ \t]*SCM__I/ { copy = $0; 
               gsub(/[ \t]*SCM__I/, "", copy); 
               gsub(/SCM__D.*$/, "", copy); 
               print copy; } 

/SCM__D/,/SCM__S/ { copy = $0; 
                 if (match(copy,/SCM__DR/)) { registering = 1; } 
                 else {registering = 0; } 
                 gsub(/.*SCM__D./,"", copy); 
                 gsub(/SCM__S.*/,"",copy); 
                 gsub(/[ \t]+/," ", copy); 
		 sub(/^[ \t]*/,"(", copy);
                 gsub(/\"/,"",copy); 
		 sub(/\([ \t]*void[ \t]*\)/,"()", copy);
		 sub(/ \(/," ",copy);
		 numargs = gsub(/SCM /,"", copy);
		 numcommas = gsub(/,/,"", copy);
		 numactuals = $2 + $3 + $4;
		 location = $5;
		 gsub(/\"/,"",location);
		 sub(/^[ \t]*/,"",location);
		 sub(/[ \t]*$/,"",location);
		 sub(/: /,":",location);
		 # Now whittle copy down to just the $1 field
		 #   (but do not use $1, since it hasn't been
                 #    altered by the above regexps)
		 gsub(/[ \t]*\|.*$/,"",copy);
		 sub(/ \)/,")",copy);
		 # Now `copy' contains the nice scheme proc "prototype", e.g.
		 # (set-car! pair value)
		 # print copy > "/dev/stderr";  # for debugging
		 proc_and_args = copy;
		 curr_function_proto = copy;
		 sub(/[^ \n]* /,"",proc_and_args);
		 sub(/\)[ \t]*/,"",proc_and_args);
		 split(proc_and_args,args," ");
		 # now args is an array of the arguments
		 # args[1] is the formal name of the first argument, etc.
		 if (numargs != numactuals && !registering) 
		   { print location ":*** `" copy "' is improperly registered as having " numactuals " arguments" > std_err; }
		 print "\n" copy (registering?")":"") > dot_doc_file ; }

/SCM__S/,/SCM__E.*$/ { copy = $0; 
                      gsub(/.*SCM__S/,"",copy); 
		      sub(/^[ \t]*"?/,"", copy);
		      sub(/\"?[ \t]*SCM__E.*$/,"", copy);
                      gsub(/\\n\\n"?/,"\n",copy);
                      gsub(/\\n"?[ \t]*$/,"",copy);
                      gsub(/\\\"[ \t]*$/,"\"",copy);
                      gsub(/[ \t]*$/,"", copy);
                      if (copy != "") { print copy > dot_doc_file }
                }

/SCM__E[ \t]/ { print "[" location "]" >> dot_doc_file; }

/\*&\*&\*&\*SCM_ARG_BETTER_BE_IN_POSITION/ { copy = $0;
         sub(/.*\*&\*&\*&\*SCM_ARG_BETTER_BE_IN_POSITION\([ \t]*/,"",copy);
         if (copy ~ /\"/) { next }
         gsub(/[ \t]*,[ \t]*/,":",copy);
         sub(/[ \t]*\).*/,"",copy);
         split(copy,argpos,":");
         argname = argpos[1];
         pos = argpos[2];
         if (pos ~ /[A-Za-z]/) { next }
         if (pos ~ /^[ \t]*$/) { next }
         if (argname ~ / /) { next }
         line = argpos[3];
#         print pos " " args[pos] " vs. " argname > "/dev/stderr";
         if (args[pos] != argname) { print filename ":" line ":*** Argument name/number mismatch in `" curr_function_proto "' -- " argname " is not formal #" pos > "/dev/stderr"; }
      }
