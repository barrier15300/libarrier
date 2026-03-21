#ifndef LIBARRIER_TEXTFILE_READER_HPP
#define LIBARRIER_TEXTFILE_READER_HPP

#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>
#include <vector>

namespace libarrier {

template<class CharT>
class basic_TextfileReader {
	using string = std::basic_string<CharT>;
	using string_view = std::basic_string_view<CharT>;

	string m_data;
	std::vector<string_view> m_lines;

public:

	basic_TextfileReader() = default;
	basic_TextfileReader(const string& path) {
		Read(path);
	}

	bool Read(const string& path) {
		std::ifstream ifs(path, std::ios::binary | std::ios::ate);

		if (!ifs.is_open()) { return false; }

		auto size = ifs.tellg();
		ifs.seekg(0, std::ios::beg);

		m_data.resize(static_cast<size_t>(size));
		ifs.read(m_data.data(), size);

		ifs.close();

		CreateIndex();

		return true;
	}
	void CreateIndex() {
		if (!HasData()) { return; }

		m_lines.clear();

		auto begin = m_data.begin();
		auto end = m_data.end();

		using size_type = decltype(std::count(begin, end, ' '));

		size_type nsize = std::count(begin, end, '\n');
		size_type rsize = std::count(begin, end, '\r');

		size_t endtypeindex = (nsize == rsize) ? (0) : ((rsize == 0) ? (1) : ((nsize == 0) ? (2) : (3)));

		constexpr string_view endcodetype[] = {"\n\r", "\n", "\r", ""};
		size_type endlinecounts[] = {nsize, nsize, rsize, 0};

		m_lines.reserve(endlinecounts[endtypeindex]);

		string_view delim = endcodetype[endtypeindex];
		string_view refdata = m_data;

		size_t prev = delim.empty() ? string_view::npos : 0;
		size_t idxbegin = -delim.size();
		size_t idxend = 0;
		while (prev != string::npos) {
			prev = idxend + delim.size();
			idxbegin = prev;
			idxend = refdata.find(delim, prev);

			m_lines.push_back(refdata.substr(idxbegin, idxend));
		}
	}

	bool HasData() const {
		return !m_data.empty();
	}
	operator bool() const {
		return HasData();
	}

	string_view operator[](size_t idx) const noexcept {
		return m_lines[idx];
	}
	string_view at(size_t idx) const {
		return m_lines.at(idx);
	}

	auto begin() -> decltype(std::begin(m_lines)) {
		return std::begin(m_lines);
	}
	auto end() -> decltype(std::end(m_lines)) {
		return std::end(m_lines);
	}
	auto begin() const -> decltype(std::begin(m_lines)) {
		return std::begin(m_lines);
	}
	auto end() const -> decltype(std::end(m_lines)) {
		return std::end(m_lines);
	}
	auto cbegin() const -> decltype(std::cbegin(m_lines)) {
		return std::cbegin(m_lines);
	}
	auto cend() const -> decltype(std::cend(m_lines)) {
		return std::cend(m_lines);
	}
	auto rbegin() -> decltype(std::rbegin(m_lines)) {
		return std::rbegin(m_lines);
	}
	auto rend() -> decltype(std::rend(m_lines)) {
		return std::rend(m_lines);
	}
	auto rbegin() const -> decltype(std::rbegin(m_lines)) {
		return std::rbegin(m_lines);
	}
	auto rend() const -> decltype(std::rend(m_lines)) {
		return std::rend(m_lines);
	}
	auto crbegin() const -> decltype(std::crbegin(m_lines)) {
		return std::crbegin(m_lines);
	}
	auto crend() const -> decltype(std::crend(m_lines)) {
		return std::crend(m_lines);
	}

	size_t find() const {
		return 0; // TODO:
	}
};

} // namespace libarrier

#endif // LIBARRIER_TEXTFILE_READER_HPP