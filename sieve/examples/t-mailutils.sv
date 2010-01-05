require "fileinto";

if header :contains ["to", "cc"] "bug-mailutils@gnu.org"
  {
    fileinto "=l.mailutils";
  }

