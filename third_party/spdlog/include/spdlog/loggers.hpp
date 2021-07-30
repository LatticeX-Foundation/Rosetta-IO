
// ==============================================================================
// Copyright 2021 The LatticeX Foundation
// This file is part of the Rosetta library.
//
// The Rosetta library is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// The Rosetta library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with the Rosetta library. If not, see <http://www.gnu.org/licenses/>.
// ==============================================================================
#pragma once
// std::string
extern template void spdlog::logger::log<std::string>(spdlog::source_loc, spdlog::level::level_enum, fmt::v6::basic_string_view<char>, const std::string&);
// unsigned long 
extern template void spdlog::logger::log<unsigned long>(spdlog::source_loc, spdlog::level::level_enum, fmt::v6::basic_string_view<char>, const unsigned long&);
// const char*
extern template void spdlog::logger::log<char const*>(spdlog::source_loc, spdlog::level::level_enum, fmt::v6::basic_string_view<char>, char const* const&);
// const char*, int
extern template void spdlog::logger::log<char const*, int>(spdlog::source_loc, spdlog::level::level_enum, fmt::v6::basic_string_view<char>, char const* const&, int const&);
// const char*, int, int
extern template void spdlog::logger::log<char const*, int, int>(spdlog::source_loc, spdlog::level::level_enum, fmt::v6::basic_string_view<char>, char const* const&, int const&, int const&);
// const char*, int, int, int
extern template void spdlog::logger::log<char const*, int, int, int>(spdlog::source_loc, spdlog::level::level_enum, fmt::v6::basic_string_view<char>, char const* const&, int const&, int const&, int const&);

// char*
extern template void spdlog::logger::log<char *>(spdlog::source_loc, spdlog::level::level_enum, fmt::v6::basic_string_view<char>, char * const&);
// char*, int
extern template void spdlog::logger::log<char *, int>(spdlog::source_loc, spdlog::level::level_enum, fmt::v6::basic_string_view<char>, char * const&, int const&);
// char*, int, int
extern template void spdlog::logger::log<char *, int, int>(spdlog::source_loc, spdlog::level::level_enum, fmt::v6::basic_string_view<char>, char * const&, int const&, int const&);
// char*, int, int, int
extern template void spdlog::logger::log<char *, int, int, int>(spdlog::source_loc, spdlog::level::level_enum, fmt::v6::basic_string_view<char>, char * const&, int const&, int const&, int const&);
// const char*, int, int, int, string
extern template void spdlog::logger::log<char const*, int, int, int, std::string>(spdlog::source_loc, spdlog::level::level_enum, fmt::v6::basic_string_view<char>, char const* const&, int const&, int const&, int const&, std::string const&);
// const char*, int, int, string
extern template void spdlog::logger::log<char const*, int, int, std::string>(spdlog::source_loc, spdlog::level::level_enum, fmt::v6::basic_string_view<char>, char const* const&, int const&, int const&, std::string const&);
// const char*, int, string
extern template void spdlog::logger::log<char const*, int, std::string>(spdlog::source_loc, spdlog::level::level_enum, fmt::v6::basic_string_view<char>, char const* const&, int const&, std::string const&);
// const char*, string
extern template void spdlog::logger::log<char const*, std::string>(spdlog::source_loc, spdlog::level::level_enum, fmt::v6::basic_string_view<char>, char const* const&, std::string const&);

// char []
extern template void spdlog::logger::log<char [1], (char (*) [1])0>(spdlog::source_loc, spdlog::level::level_enum, char const (&) [1]);
extern template void spdlog::logger::log<char [2], (char (*) [2])0>(spdlog::source_loc, spdlog::level::level_enum, char const (&) [2]);
extern template void spdlog::logger::log<char [3], (char (*) [3])0>(spdlog::source_loc, spdlog::level::level_enum, char const (&) [3]);
extern template void spdlog::logger::log<char [4], (char (*) [4])0>(spdlog::source_loc, spdlog::level::level_enum, char const (&) [4]);
extern template void spdlog::logger::log<char [5], (char (*) [5])0>(spdlog::source_loc, spdlog::level::level_enum, char const (&) [5]);
extern template void spdlog::logger::log<char [6], (char (*) [6])0>(spdlog::source_loc, spdlog::level::level_enum, char const (&) [6]);
extern template void spdlog::logger::log<char [7], (char (*) [7])0>(spdlog::source_loc, spdlog::level::level_enum, char const (&) [7]);
extern template void spdlog::logger::log<char [8], (char (*) [8])0>(spdlog::source_loc, spdlog::level::level_enum, char const (&) [8]);
extern template void spdlog::logger::log<char [9], (char (*) [9])0>(spdlog::source_loc, spdlog::level::level_enum, char const (&) [9]);
extern template void spdlog::logger::log<char [10], (char (*) [10])0>(spdlog::source_loc, spdlog::level::level_enum, char const (&) [10]);
extern template void spdlog::logger::log<char [11], (char (*) [11])0>(spdlog::source_loc, spdlog::level::level_enum, char const (&) [11]);
extern template void spdlog::logger::log<char [12], (char (*) [12])0>(spdlog::source_loc, spdlog::level::level_enum, char const (&) [12]);
extern template void spdlog::logger::log<char [13], (char (*) [13])0>(spdlog::source_loc, spdlog::level::level_enum, char const (&) [13]);
extern template void spdlog::logger::log<char [14], (char (*) [14])0>(spdlog::source_loc, spdlog::level::level_enum, char const (&) [14]);
extern template void spdlog::logger::log<char [15], (char (*) [15])0>(spdlog::source_loc, spdlog::level::level_enum, char const (&) [15]);
extern template void spdlog::logger::log<char [16], (char (*) [16])0>(spdlog::source_loc, spdlog::level::level_enum, char const (&) [16]);
extern template void spdlog::logger::log<char [17], (char (*) [17])0>(spdlog::source_loc, spdlog::level::level_enum, char const (&) [17]);
extern template void spdlog::logger::log<char [18], (char (*) [18])0>(spdlog::source_loc, spdlog::level::level_enum, char const (&) [18]);
extern template void spdlog::logger::log<char [19], (char (*) [19])0>(spdlog::source_loc, spdlog::level::level_enum, char const (&) [19]);
extern template void spdlog::logger::log<char [20], (char (*) [20])0>(spdlog::source_loc, spdlog::level::level_enum, char const (&) [20]);
extern template void spdlog::logger::log<char [21], (char (*) [21])0>(spdlog::source_loc, spdlog::level::level_enum, char const (&) [21]);
extern template void spdlog::logger::log<char [22], (char (*) [22])0>(spdlog::source_loc, spdlog::level::level_enum, char const (&) [22]);
extern template void spdlog::logger::log<char [23], (char (*) [23])0>(spdlog::source_loc, spdlog::level::level_enum, char const (&) [23]);
extern template void spdlog::logger::log<char [24], (char (*) [24])0>(spdlog::source_loc, spdlog::level::level_enum, char const (&) [24]);
extern template void spdlog::logger::log<char [25], (char (*) [25])0>(spdlog::source_loc, spdlog::level::level_enum, char const (&) [25]);
extern template void spdlog::logger::log<char [26], (char (*) [26])0>(spdlog::source_loc, spdlog::level::level_enum, char const (&) [26]);
extern template void spdlog::logger::log<char [27], (char (*) [27])0>(spdlog::source_loc, spdlog::level::level_enum, char const (&) [27]);
extern template void spdlog::logger::log<char [28], (char (*) [28])0>(spdlog::source_loc, spdlog::level::level_enum, char const (&) [28]);
extern template void spdlog::logger::log<char [29], (char (*) [29])0>(spdlog::source_loc, spdlog::level::level_enum, char const (&) [29]);
extern template void spdlog::logger::log<char [30], (char (*) [30])0>(spdlog::source_loc, spdlog::level::level_enum, char const (&) [30]);
extern template void spdlog::logger::log<char [31], (char (*) [31])0>(spdlog::source_loc, spdlog::level::level_enum, char const (&) [31]);
extern template void spdlog::logger::log<char [32], (char (*) [32])0>(spdlog::source_loc, spdlog::level::level_enum, char const (&) [32]);
extern template void spdlog::logger::log<char [33], (char (*) [33])0>(spdlog::source_loc, spdlog::level::level_enum, char const (&) [33]);
extern template void spdlog::logger::log<char [34], (char (*) [34])0>(spdlog::source_loc, spdlog::level::level_enum, char const (&) [34]);
extern template void spdlog::logger::log<char [35], (char (*) [35])0>(spdlog::source_loc, spdlog::level::level_enum, char const (&) [35]);
extern template void spdlog::logger::log<char [36], (char (*) [36])0>(spdlog::source_loc, spdlog::level::level_enum, char const (&) [36]);
extern template void spdlog::logger::log<char [37], (char (*) [37])0>(spdlog::source_loc, spdlog::level::level_enum, char const (&) [37]);
extern template void spdlog::logger::log<char [38], (char (*) [38])0>(spdlog::source_loc, spdlog::level::level_enum, char const (&) [38]);
extern template void spdlog::logger::log<char [39], (char (*) [39])0>(spdlog::source_loc, spdlog::level::level_enum, char const (&) [39]);
extern template void spdlog::logger::log<char [40], (char (*) [40])0>(spdlog::source_loc, spdlog::level::level_enum, char const (&) [40]);
extern template void spdlog::logger::log<char [41], (char (*) [41])0>(spdlog::source_loc, spdlog::level::level_enum, char const (&) [41]);
extern template void spdlog::logger::log<char [42], (char (*) [42])0>(spdlog::source_loc, spdlog::level::level_enum, char const (&) [42]);
extern template void spdlog::logger::log<char [43], (char (*) [43])0>(spdlog::source_loc, spdlog::level::level_enum, char const (&) [43]);
extern template void spdlog::logger::log<char [44], (char (*) [44])0>(spdlog::source_loc, spdlog::level::level_enum, char const (&) [44]);
extern template void spdlog::logger::log<char [45], (char (*) [45])0>(spdlog::source_loc, spdlog::level::level_enum, char const (&) [45]);
extern template void spdlog::logger::log<char [46], (char (*) [46])0>(spdlog::source_loc, spdlog::level::level_enum, char const (&) [46]);
extern template void spdlog::logger::log<char [47], (char (*) [47])0>(spdlog::source_loc, spdlog::level::level_enum, char const (&) [47]);
extern template void spdlog::logger::log<char [48], (char (*) [48])0>(spdlog::source_loc, spdlog::level::level_enum, char const (&) [48]);
extern template void spdlog::logger::log<char [49], (char (*) [49])0>(spdlog::source_loc, spdlog::level::level_enum, char const (&) [49]);
extern template void spdlog::logger::log<char [50], (char (*) [50])0>(spdlog::source_loc, spdlog::level::level_enum, char const (&) [50]);
extern template void spdlog::logger::log<char [51], (char (*) [51])0>(spdlog::source_loc, spdlog::level::level_enum, char const (&) [51]);
extern template void spdlog::logger::log<char [52], (char (*) [52])0>(spdlog::source_loc, spdlog::level::level_enum, char const (&) [52]);
extern template void spdlog::logger::log<char [53], (char (*) [53])0>(spdlog::source_loc, spdlog::level::level_enum, char const (&) [53]);
extern template void spdlog::logger::log<char [54], (char (*) [54])0>(spdlog::source_loc, spdlog::level::level_enum, char const (&) [54]);
extern template void spdlog::logger::log<char [55], (char (*) [55])0>(spdlog::source_loc, spdlog::level::level_enum, char const (&) [55]);
extern template void spdlog::logger::log<char [56], (char (*) [56])0>(spdlog::source_loc, spdlog::level::level_enum, char const (&) [56]);
extern template void spdlog::logger::log<char [57], (char (*) [57])0>(spdlog::source_loc, spdlog::level::level_enum, char const (&) [57]);
extern template void spdlog::logger::log<char [58], (char (*) [58])0>(spdlog::source_loc, spdlog::level::level_enum, char const (&) [58]);
extern template void spdlog::logger::log<char [59], (char (*) [59])0>(spdlog::source_loc, spdlog::level::level_enum, char const (&) [59]);
extern template void spdlog::logger::log<char [60], (char (*) [60])0>(spdlog::source_loc, spdlog::level::level_enum, char const (&) [60]);
extern template void spdlog::logger::log<char [61], (char (*) [61])0>(spdlog::source_loc, spdlog::level::level_enum, char const (&) [61]);
extern template void spdlog::logger::log<char [62], (char (*) [62])0>(spdlog::source_loc, spdlog::level::level_enum, char const (&) [62]);
extern template void spdlog::logger::log<char [63], (char (*) [63])0>(spdlog::source_loc, spdlog::level::level_enum, char const (&) [63]);
extern template void spdlog::logger::log<char [64], (char (*) [64])0>(spdlog::source_loc, spdlog::level::level_enum, char const (&) [64]);
extern template void spdlog::logger::log<char [65], (char (*) [65])0>(spdlog::source_loc, spdlog::level::level_enum, char const (&) [65]);
extern template void spdlog::logger::log<char [66], (char (*) [66])0>(spdlog::source_loc, spdlog::level::level_enum, char const (&) [66]);
extern template void spdlog::logger::log<char [67], (char (*) [67])0>(spdlog::source_loc, spdlog::level::level_enum, char const (&) [67]);
extern template void spdlog::logger::log<char [68], (char (*) [68])0>(spdlog::source_loc, spdlog::level::level_enum, char const (&) [68]);
extern template void spdlog::logger::log<char [69], (char (*) [69])0>(spdlog::source_loc, spdlog::level::level_enum, char const (&) [69]);
extern template void spdlog::logger::log<char [70], (char (*) [70])0>(spdlog::source_loc, spdlog::level::level_enum, char const (&) [70]);
extern template void spdlog::logger::log<char [71], (char (*) [71])0>(spdlog::source_loc, spdlog::level::level_enum, char const (&) [71]);
extern template void spdlog::logger::log<char [72], (char (*) [72])0>(spdlog::source_loc, spdlog::level::level_enum, char const (&) [72]);
extern template void spdlog::logger::log<char [73], (char (*) [73])0>(spdlog::source_loc, spdlog::level::level_enum, char const (&) [73]);
extern template void spdlog::logger::log<char [74], (char (*) [74])0>(spdlog::source_loc, spdlog::level::level_enum, char const (&) [74]);
extern template void spdlog::logger::log<char [75], (char (*) [75])0>(spdlog::source_loc, spdlog::level::level_enum, char const (&) [75]);
extern template void spdlog::logger::log<char [76], (char (*) [76])0>(spdlog::source_loc, spdlog::level::level_enum, char const (&) [76]);
extern template void spdlog::logger::log<char [77], (char (*) [77])0>(spdlog::source_loc, spdlog::level::level_enum, char const (&) [77]);
extern template void spdlog::logger::log<char [78], (char (*) [78])0>(spdlog::source_loc, spdlog::level::level_enum, char const (&) [78]);
extern template void spdlog::logger::log<char [79], (char (*) [79])0>(spdlog::source_loc, spdlog::level::level_enum, char const (&) [79]);
extern template void spdlog::logger::log<char [80], (char (*) [80])0>(spdlog::source_loc, spdlog::level::level_enum, char const (&) [80]);
extern template void spdlog::logger::log<char [81], (char (*) [81])0>(spdlog::source_loc, spdlog::level::level_enum, char const (&) [81]);
extern template void spdlog::logger::log<char [82], (char (*) [82])0>(spdlog::source_loc, spdlog::level::level_enum, char const (&) [82]);
extern template void spdlog::logger::log<char [83], (char (*) [83])0>(spdlog::source_loc, spdlog::level::level_enum, char const (&) [83]);
extern template void spdlog::logger::log<char [84], (char (*) [84])0>(spdlog::source_loc, spdlog::level::level_enum, char const (&) [84]);
extern template void spdlog::logger::log<char [85], (char (*) [85])0>(spdlog::source_loc, spdlog::level::level_enum, char const (&) [85]);
extern template void spdlog::logger::log<char [86], (char (*) [86])0>(spdlog::source_loc, spdlog::level::level_enum, char const (&) [86]);
extern template void spdlog::logger::log<char [87], (char (*) [87])0>(spdlog::source_loc, spdlog::level::level_enum, char const (&) [87]);
extern template void spdlog::logger::log<char [88], (char (*) [88])0>(spdlog::source_loc, spdlog::level::level_enum, char const (&) [88]);
extern template void spdlog::logger::log<char [89], (char (*) [89])0>(spdlog::source_loc, spdlog::level::level_enum, char const (&) [89]);
extern template void spdlog::logger::log<char [90], (char (*) [90])0>(spdlog::source_loc, spdlog::level::level_enum, char const (&) [90]);
extern template void spdlog::logger::log<char [91], (char (*) [91])0>(spdlog::source_loc, spdlog::level::level_enum, char const (&) [91]);
extern template void spdlog::logger::log<char [92], (char (*) [92])0>(spdlog::source_loc, spdlog::level::level_enum, char const (&) [92]);
extern template void spdlog::logger::log<char [93], (char (*) [93])0>(spdlog::source_loc, spdlog::level::level_enum, char const (&) [93]);
extern template void spdlog::logger::log<char [94], (char (*) [94])0>(spdlog::source_loc, spdlog::level::level_enum, char const (&) [94]);
extern template void spdlog::logger::log<char [95], (char (*) [95])0>(spdlog::source_loc, spdlog::level::level_enum, char const (&) [95]);
extern template void spdlog::logger::log<char [96], (char (*) [96])0>(spdlog::source_loc, spdlog::level::level_enum, char const (&) [96]);
extern template void spdlog::logger::log<char [97], (char (*) [97])0>(spdlog::source_loc, spdlog::level::level_enum, char const (&) [97]);
extern template void spdlog::logger::log<char [98], (char (*) [98])0>(spdlog::source_loc, spdlog::level::level_enum, char const (&) [98]);
extern template void spdlog::logger::log<char [99], (char (*) [99])0>(spdlog::source_loc, spdlog::level::level_enum, char const (&) [99]);
extern template void spdlog::logger::log<char [100], (char (*) [100])0>(spdlog::source_loc, spdlog::level::level_enum, char const (&) [100]);

