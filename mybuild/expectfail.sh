#!/bin/bash

$@
EXIT_CODE=$?

if [ "$EXIT_CODE" -eq "0" ]; then
   echo "!!!!!!!!!Negative test UNEXPECTEDLY passed!!!!!!!!!";
   exit 1;
else
   echo "Negative test successful!"
   exit 0
fi

