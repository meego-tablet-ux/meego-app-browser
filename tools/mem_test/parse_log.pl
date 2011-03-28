#!/usr/bin/perl -w
# usage: ./parse_log memory_trace.log

sub next_log(){
  while(<F>){
    next unless /<<</;
    $main::i = 0;
    $main::loaded = 0;
    last;
  }
}

my (%a, %b);
our ($i, $loaded) = (0, 0);
my $N = 7;
for(1 ... $N){
  $b{$_} = 0;
}
open F, "<$ARGV[0]";
while(<F>){
  if(/>>>/){
    $i = 0;
    $loaded = 0;
    next;
  }
  if(/<<</){
    next if $i < $N;
    /<<<\s+(\d+?)\s/;
    my $x = $1;
    next if $x<100000000;
#    print "x is $x\n";
    $x = int($x/10000000)*10;
    $a{$x} += 1; 
    $b{$loaded} += 1;
#    print "i=$i, loaded=$loaded\n";
    next;
  }

  next if( !/^\s+Page/ );
  next_log() if /loading$/; 
  $loaded++ if /loaded$/;
  $i++;
}

open F, ">mem";
for(sort keys %a){
  print "$_\t$a{$_}\n";
  print F "$_\t$a{$_}\n";
}
close F;
print "=========\n";
open F, ">tab";
for(1 ... $N){
  print "$_\t$b{$_}\n";
  print F "$_\t$b{$_}\n";
}
close F;

$"=", ";
my @r = sort keys %a;
my ($min, $max) = ($r[0]-10, $r[@r-1]+10);
my $y = int((sort {$a<$b} values %a)[0]/5);

open R, ">mem.plt";
print R <<EOF
set xlabel 'Memory (M)' font "URWPalladioL-Bold,20"
set ylabel 'Number of samples' font "URWPalladioL-Bold,20"
set xtics nomirror (@r)
set ytics nomirror $y
set border 3
set style fill solid 1.0
set boxwidth 0.5 relative
set terminal post eps color solid enh
set output "mem.ps"
plot [x=$min:$max] "mem" with boxes
EOF
;
close R;

($min, $max) = (0, $N+1);
@r = sort keys %b;
$y = int((sort {$a<$b} values %b)[0]/5);

open R, ">tab.plt";
print R <<EOF
set xlabel 'Number of Active Tabs' font "URWPalladioL-Bold,20"
set ylabel 'Number of samples' font "URWPalladioL-Bold,20"
set xtics nomirror (@r)
set ytics nomirror $y
set border 3
set style fill solid 1.0
set boxwidth 0.5 relative
set terminal post eps color solid enh
set output "tab.ps"
plot [x=$min:$max] "tab" with boxes
EOF
;
close R;

#generating figures
system "gnuplot mem.plt";
system "gnuplot tab.plt";

system "rm -rf mem mem.plt tab tab.plt";
