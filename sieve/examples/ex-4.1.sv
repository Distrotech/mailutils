require "reject";

if header :contains "from" "coyote@desert.example.org"
  {
    reject
     "I am not taking mail from you, and I don't want
      your birdseed, either!";
  }

