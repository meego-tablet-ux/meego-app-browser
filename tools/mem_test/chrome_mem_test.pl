#!/usr/bin/perl -w
# x.pl
use Getopt::Long;

$\="\n";
my $tab_num = 7;
my $threshold = 4000000;
my $sitelist = "sitelist";
my $result = GetOptions ("tabnumber=i" => \$tab_num,
                         "threshold=i" => \$threshold, 
                         "sitelist=s"  => \$sitelist);
&usage($0) unless $result && !@ARGV;                       

print "tab_num = $tab_num\nthreshold = $threshold\nsitelist = $sitelist";

my @a;
open F, "<$sitelist" or die "Can't open sitelist";
while(<F>){
  chomp;
  push @a, $_;
}
close F;

my $N = $tab_num;
open L, ">>running_log";
while(1){
  system("echo meego | sudo -S sysctl -w vm.drop_caches=3 &>/dev/null");
  my $sites;
  my $i = 0;
  my @url;
  `free` =~ /cache:\s+(\d+)/;
  print L ">>> before chromium launching: $1 kB";
  while( $i++ < $N ){
    my $x;
    do{ $x = int(rand @a); }while( scalar grep {$x==$_} @url);
    push @url, $x;
    `env CHROME_MEM_TRACE=1 ./chrome --process-per-tab --memory-threshold=$threshold --disable-accelerated-compositing $a[$x] &>/dev/null &`;
    sleep 20;
    print L "opened $a[$x]";
  }
  `free` =~ /cache:\s+(\d+)/;
  print L "<<< after chromium opened($N tabs): $1 kB";
  system("killall -9 chrome");
  sleep 3;
}
close L;

sub usage(){
  print "Usage:\n";
  print "$_[0] --tabnumber x --threshold y --sitelist sitelist_file\n";
  exit;
}
