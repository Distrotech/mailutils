require "fileinto";

if anyof (
     not exists ["From", "Date"],
     header :contains "from" "fool@example.edu"
     )
  {
    discard;
  }
if header :contains "from" "coyote"
  {
    fileinto "popbox"; # "pop://sam:passwed@fw";
  }
