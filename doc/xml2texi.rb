#!/usr/local/bin/ruby -w

if ! ARGV[0]
 print <<USAGE
Usage: xml2texi.rb xml-file

The texinfo for function documentation found in xml-file
is dumped to stdout.
USAGE
exit 0
end

$\ = "\n"

require "rexml/document"
include REXML

# Utility functions

def printPara(p)
  if p.name != "para"
    raise "printPara: " + p.name + " not a paragraph!\n"
  end

  p.each do |c|
    if c.kind_of? Text
      print c
    else
      if c.name == "parameterlist"
        # print "!! skipping parameterlist"
      else
        print "\n!! Unknown element type: ", c.name, "\n"
      end
    end
  end
  print "\n"
  print "\n"
end

# Open the document, compress whitespace.

doc = Document.new File.new(ARGV[0]), { :compress_whitespace => :all }

# <?xml version='1.0' encoding='ISO-8859-1' standalone='yes'?>
# <doxygen version="1.2.15-20020430">
#   <compounddef id="smtp_8c" kind="file">
# 	-- ...

#print "doc.whitespace: ", doc.whitespace
#print "doc.version: ", doc.version
#print "doc.encoding: ", doc.encoding
#print "doc.standalone: ", doc.standalone?


# Input:
#
#       <sectiondef kind="func">
#       <memberdef kind="function" id="smtp_8c_1a7" virt="normal" prot="public" const="no" volatile="no">
#         <type>int</type>
#         <name>smtp_open</name>
#         <param>
#           <type>mailer_t</type>
#           <defname>mailer</defname>
#         </param>
#         <param>
#           <type>int</type>
#           <defname>flags</defname>
#         </param>
#         <briefdescription>
# <para>Open an SMTP mailer. </para>
#         </briefdescription>
#         <detaileddescription>
# <para>An SMTP mailer must be opened before any messages can be sent. <parameterlist kind="param"><title>Parameters: </title><parametername>mailer</parametername><parameterdescription><para>the mailer created by smtp_create() </para>
# </parameterdescription><parametername>flags</parametername><parameterdescription><para>the mailer flags </para>
# </parameterdescription></parameterlist></para>
#         </detaileddescription>
#         <location file="/home/sam/p/gnu/mailutils/mailbox/smtp.c" line="223" bodystart="289" bodyend="434"/>
#       </memberdef>

# - Why do I always get XML, even for undocumented functions and macros?
# - Why is # there no markup that I can use to determine that the function
# was undocumented?
# - Why sometimes declname, sometimes defname, in param?
# - why is the  parameter list jammed into the last paragraph of the
# detailed description?
# - Why is there always a detailed description, even when there isn't?
# - Why the duplication beteen the beginning list of <param> (which is
# lacking the <parameterdescription>, and the <parameterlist>?
# - Why is parameterlist just an alternating list of names and descriptions,
# with no markup to bind a name to a description?

# Overall: this appears to be half visual markup, and half logical. If
# it was entirely visual, there would be the one-line summary of the
# documented functions, followed by the details.
# If it was entirely markup, then the
# parameter info wouldn't be split into two different locations.



# Output:
# 
# @deftypefun int smtp_open (
#   mailer_t @var{mailer},
#   int @var{flags})
# Open an SMTP mailer.
# An SMTP mailer must be opened before any messages can be sent.
# 
# Parameters:
# @itemize
# @item @code{mailer} - the mailer created by smtp_create()
# @item @code{flags} - the mailer flags
# @end itemize
# @end deftypefun


doc.elements.each("*/*/sectiondef[@kind='func']") { |e|
  print "section: ", e.name, "/", e.attributes["kind"]

  e.elements.each("memberdef") { |m|
    # If there is no brief description, don't document this member.
		next unless m.elements["briefdescription"].elements["para"]

    $\ = ""
    print "@deftypefun "
    if m.elements["type"].text
        print m.elements["type"].text, " "
    else
        print "int "
    end

    print m.elements["name"].text, " (\n  "

    first = true

    m.elements.each("param") { |p|
      if(first)
        first = false
      else
        print ",\n  "
      end

      p.elements["type"].each { |c|
        if c.kind_of? Text
          print c, " "
        else
          c.each { |c2|
            if c2.kind_of? Text
              print c2, " "
            end
          }
        end
      }
      name = p.elements["declname"];
			if ! name
        name = p.elements["defname"];
			end
      if name
        print "@var{", name.text, "}"
      end
    }
    print ")\n"

		print m.elements["briefdescription"].elements["para"].text

    dd = m.elements["detaileddescription"]

    if dd
      print "\n"
      dd.each do |dde|
        if dde.kind_of? Text
          # Top level Text is empty.
        else
          # We should have a string of "para" elements, the last of which
          # has the parameter list.
          printPara dde
        end
      end

      print "Parameters:\n"
      print "@itemize\n"

      dd.elements.each("*/parameterlist/*") do |pl|
        if pl.name == "title"
          # ignore
        elsif pl.name  == "parametername"
          print "@item @code{", pl.text, "}"
        elsif pl.name  == "parameterdescription"
          if pl.elements["para"]
            print " - ", pl.elements["para"].text, "\n"
          end
        end
      end

      print "@end itemize\n"

    end

    print "@end deftypefun\n"
    print "\n"
  }
}

