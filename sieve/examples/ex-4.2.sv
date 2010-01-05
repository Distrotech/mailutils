require "fileinto";
if header :contains ["from"] "coyote"
  {
    fileinto "INBOX.harassment";
  }

