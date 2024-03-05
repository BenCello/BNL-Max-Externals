//#include <iostream>
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators_adapters.hpp>
#include <catch2/generators/catch_generators_random.hpp>
#include <catch2/generators/catch_generators_range.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <iostream>
#include <fstream>
#include <vector>
#include <numeric>
#include <algorithm> // for Windows

#include "bc.virfun.h"

void load_midifreqs(std::vector<float>& in_vector)
{
  float freq;
  std::ifstream f_midif ("midi_freqs.txt");

  if (f_midif.is_open()) {
    int i = 0;
    while (f_midif >> freq) {
        in_vector[i++] = freq;
    }
    f_midif.close();
  }
  else std::cout << "Unable to open file"<<std::endl; 
}

TEST_CASE ("Tests for [bc.virfun]", "[bc.virfun]")
{
  std::vector<float> v_midif( 128 );
  load_midifreqs(v_midif);

  SECTION("midi2freq_approx") {
    REQUIRE(midi2freq_approx(0) == 1);
    REQUIRE_THAT(midi2freq_approx(1), Catch::Matchers::WithinAbs(1.05946, 0.0001));
  }

  auto i = GENERATE(take(10, random(0, 127)));
  SECTION("midi2freq") {
    std::cout<<"note "<<i;
    REQUIRE_THAT(midi2freq(i), Catch::Matchers::WithinAbs(v_midif[i], 0.0001));
    std::cout<<" = ";
  }
  SECTION("freq2midi") {
    std::cout<<v_midif[i]<<"Hz";
    REQUIRE_THAT(freq2midi(v_midif[i]), Catch::Matchers::WithinAbs(i, 0.0001));
    std::cout<<" ";
  }
  SECTION("rec_virfun") {
    // One frequency = itself
    double single_freq = midi2freq(i);
    REQUIRE(rec_virfun(&single_freq, &single_freq+1, 0.1, single_freq, midi2freq_approx(0)) == midi2freq(i));

    double two_freqs [2];
    two_freqs[0] = round(midi2freq(i));
    auto j = Catch::Generators::random(0, 127).get();
    two_freqs[1] = round(midi2freq(j));
    std::cout<<std::endl<<"f1="<<two_freqs[0]<<" f2="<<two_freqs[1];
    // Two integer frequencies = greatest common divisor
    REQUIRE(round(rec_virfun(two_freqs, two_freqs+2, 0.1, two_freqs[0], midi2freq_approx(0))) == std::gcd((int)round(two_freqs[0]), (int)round(two_freqs[1])));
    std::cout<<" => "<<std::gcd((int)round(two_freqs[0]), (int)round(two_freqs[1]))<<std::endl;

    // Generate vectors of (sub-)harmonics
    auto rand1 = Catch::Generators::random(1, 8); // multiplicator
    auto rand2 = Catch::Generators::random(0, 1); // sub or overtone
    std::vector<double> freqs(8, midi2freq(i));
    for (auto &f : freqs) {
      f = rand2.get() ? f * rand1.get() : f / (rand1.get() % 3) ;
      rand1.next(); rand2.next();
    }
    // sort
    std::sort(freqs.begin(), freqs.end());
    // delete duplicates
    auto last = std::unique(freqs.begin(), freqs.end());
    freqs.erase(last, freqs.end());
    // remove if superior to 20000Hz
    freqs.erase(std::remove_if(freqs.begin(), freqs.end(), [&](const float& f) { return f > 20000.; }), freqs.end());
    // print the vector
    for (auto f : freqs)
      std::cout<<f<<" ";
    std::cout<<std::endl;
    // Constructed vectors of a fundamental frequency
    // => this fundamental if present, no subharmonics and not a singular vector
    REQUIRE(rec_virfun(&(*freqs.begin()), &(*freqs.end()), 0.1, *freqs.begin(), midi2freq_approx(0)) == (((*freqs.begin()) < midi2freq(i) || freqs.size() == 1) ? *freqs.begin() : midi2freq(i)));
  }
}
