#!/bin/sh -e
# Run this to get a template for the updated ChangeLog from git commit log
# before releasing a new upstream tarball.
#
# It may be desirable to perform some manual touch-ups before committing
# the updated ChangeLog.

LAST_TAG=$(head -1 ChangeLog|sed -nE "s/^.* ([0-9a-f]+)$/\1/p")
TOP_TAG=$(git rev-parse --short HEAD)
if [ "$LAST_TAG" != "$TOP_TAG" ]; then
    mv ChangeLog ChangeLog.keep
    git log --date=short \
            --abbrev-commit \
            --format="format:%cd %an <%ae> %h%n%n  * %s%n%w(72,4,4)%b" \
            ${LAST_TAG}..HEAD >ChangeLog
    echo "" >> ChangeLog
    cat ChangeLog.keep >> ChangeLog
    rm  ChangeLog.keep
fi
