require [ "redirect" ];

if header :contains ["From"] ["coyote"]
  {
    redirect "acm@example.edu";
  }
elsif header :contains "Subject" "$$$"
  {
    redirect "postmaster@example.edu";
  }
else
  {
    redirect "field@example.edu";
  }
