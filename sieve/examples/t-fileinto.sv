require "fileinto";

if allof(
		size :over 10 ,
		exists "x-caffeine"
	)
  {
    fileinto "jetfuel";
  }
else
  {
    fileinto "decaf";
  }

