# header :is ["X-Caffeine"] [""]         => false
# header :contains ["X-Caffeine"] [""]   => true

# These should be true, then, and not affect the test mbox.

if header :is ["X-Caffeine"] [""]
  {
    discard;
  }

if not header :contains ["X-Caffeine"] [""]
  {
    discard;
  }

