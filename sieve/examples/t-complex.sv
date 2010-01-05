require [ "fileinto", "redirect" ];

require [ "comparator-i;octet", "comparator-i;ascii-casemap" ];

# Idiom to determine existence of a header.
if header :contains [ "x-sweetheart", "x-unlikely" ] ""
  {
    stop;
  }

if header :is "return-PATH" "<>"
  {
    fileinto "mbox.complex.null-path";
  }

if header :matches "newsgroups" "comp.os.*"
  {
    fileinto "mbox.complex.news-comp-os";
  }
elsif exists "newsgroups"
  {
    fileinto "mbox.complex.news";
  }

if size :over 10000 
  {
    fileinto "mbox.complex.large";
    #  redirect "root@microsoft.com";
  }
elsif allof ( size :over 2000 , size :under 10000 )
  {
    fileinto "mbox.complex.largeish";
  }
elsif size :over 100
  {
    fileinto "mbox.complex.smallish";
  }
else
  {
    # Too small to bother reading.
    discard;
  }

if exists "x-redirect"
  {
    redirect "sroberts@uniserve.com";
  }

# if header :contains ["to", "cc"] "bug-mailutils@gnu.org"
#   {
#     fileinto "./mbox.mailutils.out";
#     keep;
# if header :contains ["to", "cc"] "bug-mailutils@gnu.org"
#   {
#     fileinto "./mbox.mailutils.out";
#     keep;
#   }
# else
#   {
#     stop;
#     discard;
#   }

# Keep (don't delete) messages, even if filed into another mailbox.
keep;
