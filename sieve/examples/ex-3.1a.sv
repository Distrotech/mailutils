require "fileinto";

if header :contains "from" "coyote"
  {
    discard;
  }
elsif header :contains ["subject"] ["$$$"]
  {
    discard;
  }
else
  {
    fileinto "INBOX";
  }
