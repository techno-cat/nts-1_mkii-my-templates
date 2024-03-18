use v5.14;
use strict;
use warnings;
use constant X_MAX => 1024;
use constant Y_MAX => 512;
use constant Y_MIN => 0;

use constant VALUE_MAX => 0x1000_0000;
use constant VALUE_MIN => 0;
use constant X_STEP => 4;

use Imager;
use List::Util qw/sum/;
use Math::Trig qw/pi tanh/;

my $margin = 10;
my $width = $margin + (X_MAX + 1) + $margin;
my $height = $margin + (100 + Y_MAX - Y_MIN + 1) + $margin;
my ( $x0, $y0 ) = ( $margin, $margin + (Y_MAX + 100 + 1) );

my $param_cnt = 64;

# 1.0 -> min になる回数でパラメータを決める
my $vol_min = 0.01;
my $n_min = 2;
my $n_max = 2 ** 10;

my $func_fb_count = sub {
    my $t = log2($n_min) + (log2($n_max) - log2($n_min)) * $_[0];
    return 2.0 ** $t;
};

my $times_min = $func_fb_count->( 0.0 );
my $gain_min = 10 ** (log10($vol_min) / $times_min);
my $times_max = $func_fb_count->( 1.0 );
my $gain_max = 10 ** (log10($vol_min) / $times_max);

my $func_param_curve = sub {
    return $_[0] ** (2 ** -0.25);
};

if ( 1 ) {
    my @samples1 = ();
    my @samples2 = ();
    my @samples3 = ();
    my @samples4 = ();
    foreach ( 0..(X_MAX / X_STEP) ) {
        my $t = $_ / (X_MAX / X_STEP);

        {
            my $times = $func_fb_count->( $t );
            my $gain = 10 ** (log10($vol_min) / $times);

            push @samples1, $t;
            push @samples2, $gain;
        }

        {
            my $t2 = $func_param_curve->($t);
            my $times = $func_fb_count->( $t2 );
            my $gain = 10 ** (log10($vol_min) / $times);

            push @samples3, $t2;
            push @samples4, $gain;
        }
    }

    write_graph( 'delay-fb.png', [
        { color => 'red' , samples => \@samples1 },
        { color => 'blue', samples => \@samples2 },
        { color => 'green', samples => \@samples3 },
        { color => 'purple', samples => \@samples4 }
    ] );
}

{
    my @tmp = map {
        my $i = $_;
        my $t = ($i + 1) / $param_cnt;

        my $t2 = $func_param_curve->($t);
        my $times = $func_fb_count->($t2);
        my $gain = 10 ** (log10($vol_min) / $times);

        if ( $i == 0 ) {
            sprintf( "%.6f /*%7s */", 0, 'off' );
        }
        else {
            my $tmp = log($gain) / log(10);
            my $times2 = log10($vol_min) / $tmp;

            sprintf( "%.6f /*%7.1f */", $gain, $times2 );
        }
    } 0..($param_cnt - 1);

    say join( ",\n", @tmp );
}

sub log2 {
    return log($_[0]) / log(2.0);
}

sub log10 {
    return log($_[0]) / log(10.0);
}

sub to_decibel {
     return 20.0 * log10($_[0]);
}

sub write_graph {
    my ( $dst_file, $src ) = @_;

    my $img = Imager->new(
        xsize => $width, ysize => $height );
    $img->box( filled => 1, color => 'white' );
    draw_graduation( $img, Imager::Color->new(192, 192, 192) );

    my %pallete = (
        orange => { hue =>  45, v => 1.0, s => 1.0, opacity => 1.0 },
        purple => { hue => 300, v => 1.0, s => 1.0, opacity => 1.0 },
        green  => { hue => 130, v => 0.7, s => 1.0, opacity => 1.0 },
        red    => { hue =>   0, v => 1.0, s => 1.0, opacity => 1.0 },
        blue   => { hue => 240, v => 1.0, s => 1.0, opacity => 1.0 },
    );

    foreach my $data ( @{$src} ) {
        my $x = 0;
        my @tmp = ();
        foreach ( @{$data->{samples}} ) {
            push @tmp, [ $x, $_ ];
            $x += X_STEP;
        }

        my $color = $data->{color};
        draw_polyline( $img, \@tmp, $pallete{$color}, 0.3 );
        plot_points( $img, \@tmp, $pallete{$color}, 0.4 );
    }

    $img->write( file => $dst_file ) or die $img->errstr;
}

sub calc_points {
    my ( $fa, $fb ) = @_;

    my @src = ();
    for (my $i=0; $i<=X_MAX; $i+=8) {
        my $t = $i / X_MAX;
        my $y = ($fa * $t) + ($fb * $t ** 5.0);
        push @src, [
            $i,
            int( ($y * 128) )
        ];
    }

    return \@src;
}

sub draw_graduation {
    my ( $img, $color ) = @_;

    {
        my $gray = Imager::Color->new( 192, 192, 192 );

        my $x = 128;#($w / 4);
        while ( $x <= X_MAX ) {
            $img->line( color => $gray,
                x1 => $x0 + $x, y1 => $y0 - Y_MIN,
                x2 => $x0 + $x, y2 => $y0 - Y_MAX );
            $x += 128;#($w / 4);
        }

        my $y = 128;#($h / 4);
        while ( $y <= Y_MAX ) {
            $img->line( color => $gray,
                x1 => $x0 + 0,     y1 => $y0 - $y,
                x2 => $x0 + X_MAX, y2 => $y0 - $y );
            $img->line( color => $gray,
                x1 => $x0 + 0,     y1 => $y0 + $y,
                x2 => $x0 + X_MAX, y2 => $y0 + $y );
            $y += 128;#($h / 4);
        }
    }

    {
        $img->line( color => 'black',
            x1 => $x0, y1 => $y0 - Y_MIN,
            x2 => $x0, y2 => $y0 - Y_MAX );

        $img->line( color => 'black',
            x1 => $x0 + 0,     y1 => $y0,
            x2 => $x0 + X_MAX, y2 => $y0 );
    }
}

sub plot_points {
    my ( $img, $data, $color, $opacity, $filled ) = @_;
    $filled //= 0;
    my $n = 1;

    my $img_dst = $img;
    if ( defined($opacity) and $opacity < 1.0 ) {
        $img_dst = Imager->new(
            xsize => $img->getwidth(), ysize => $img->getheight(), channels => 4 );
    }

    foreach my $pt ( @{$data} ) {
        my ( $x, $y ) = ( $pt->[0], $pt->[1] );
        # $y /= (VALUE_MAX / Y_MAX);
        $y *= Y_MAX;
        $y = ( $y < .0 ) ? int($y - .5) : int($y + .5);
        #printf( "%6.3f, %6.3f\n", $x, $y );

        $img_dst->box(
            xmin => $x0 + $x - $n, ymin => $y0 - $y - $n,
            xmax => $x0 + $x + $n, ymax => $y0 - $y + $n,
            color => $color, filled => $filled );
    }

    if ( $img != $img_dst ) {
        $img->compose(
            src => $img_dst, opacity => $opacity );
    }
}

sub draw_polyline {
    my ( $img, $data, $color, $opacity ) = @_;

    my $img_dst = $img;
    if ( defined($opacity) and $opacity < 1.0 ) {
        $img_dst = Imager->new(
            xsize => $img->getwidth(), ysize => $img->getheight(), channels => 4 );
    }

    my @points = map {
        my ( $x, $y ) = ( $_->[0], $_->[1] );
        $y /= (VALUE_MAX / Y_MAX);
        $y = ( $y < .0 ) ? int($y - .5) : int($y + .5);
        [ $x0 + $x, $y0 - $y ];
    } @{$data};

    $img_dst->polyline( points => \@points, color => $color );

    if ( $img != $img_dst ) {
        $img->compose(
            src => $img_dst, opacity => $opacity );
    }
}
