#!/usr/local/bin/ruby

# データベースがメモリ中に展開された時の大きさを概算する
# malloc のヘッダなどは考慮していないので、最悪この倍ぐらいを考えるといいかも

FileName = ARGV[0];  # last_record1_ を指定する

xstrs = Array[];
keys = Array[];
nSection = 0;
nColumns = 0;
nValues = 0;
nKeys = 0;
nKeysLen = 0;
nIntern = 0;
nInternLen = 0;
nUnintern = 0;
nUninternLen = 0;

f = File.new(FileName, "r");
f.each() { |l|
	if (l =~ /^---/) then
		# section の先頭
		nSection = nSection + 1;
		next;
	end
	nColumns = nColumns + 1;
	if (l =~ /^[+-]/) then
		# LRUのフラグ
		l = l[1 .. -1];
	end
	a = l.split(/\s+/);
	keys.push a.shift;
	a.each { |s|
		nValues = nValues + 1;
		if (s =~ /^\"([^"]+)\"$/) then
			xstrs.push $1;
		end
	}
}
nUnintern = xstrs.length;
nUninternLen = xstrs.join().length;
xstrs.uniq!;
nIntern = xstrs.length;
nInternLen = xstrs.join().length;
nKeys = keys.length;
nKeysLen = keys.join().length;

print "section: ", nSection, "\n";
print "column: ", nColumns, "\n";
print "keys: ", nKeys, "\n";
print "keys length: ", nKeysLen, "\n";
print "values: ", nValues, "\n";
print "interned xstr: ", nIntern, "\n";
print "interned xstr total length: ", nInternLen, "\n";
print "(no intern simulation) xstr: ", nUnintern, "\n";
print "(no intern simulation) total length: ", nUninternLen, "\n";
print "total size: ", (nColumns + nIntern) * 48 + nInternLen * 2 + nValues * 8 + + nKeysLen * 2, "\n";
