#!/usr/bin/env perl

rename("gcanna.t", "gcanna.t.orig");
open(INPUT, "gcanna.t.orig") || die "$!";
open(OUTPUT, ">>gcanna.t");
while (<INPUT>) {
    s/#KYme/#KY/og;
    s/#KYmime/#KY/og;
    print OUTPUT;
}
close(INPUT);
close(OUTPUT);
