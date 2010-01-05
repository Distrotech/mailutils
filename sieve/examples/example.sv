# Example sieve script.

require [ "fileinto", "redirect" ];
  
if size :over 20
  {
    fileinto "/home/sam/p/gnu/mailutils/cmu-sieve/sv/inbox";
  }
  
if address :domain :is "to" "uwaterloo.ca"
  {
    redirect "dom@is.uw";
  }
  
if header :is "Status" "RO"
  {
    redirect "status@is.ro";
  }
  
keep;
  
