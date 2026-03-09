#ifndef LIBARRIER_TEXTFILE_READER_HPP
#define LIBARRIER_TEXTFILE_READER_HPP

#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>
#include <vector>

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

		if (!ifs.is_open()) {
			return false;
		}

		auto size = ifs.tellg();
		ifs.seekg(0, std::ios::beg);

		m_data.resize(static_cast<size_t>(size));
		ifs.read(m_data.data(), size);

		ifs.close();

		CreateIndex();

		return true;
	}
	void CreateIndex() {
		if (HasData()) {
			return;
		}

		auto begin = m_data.begin();
		auto end = m_data.end();

		auto nsize = std::count(begin, end, '\n');
		auto rsize = std::count(begin, end, '\r');

		m_lines.resize(nsize);

		string_view delim = 
			(nsize == rsize) ? ("\n\r") :
			((rsize == 0) ? ("\n") :
			((nsize == 0) ? ("\r") : ("")));
		string_view refdata = m_data;

		size_t prev = delim.empty() ? string_view::npos : 0;
		while (prev != string::npos) {
			size_t idxbegin = prev;
			size_t idxend = m_data.find(delim, idxend + 1);
			prev = idxend;
			
			m_lines.push_back(refdata.substr(idxbegin, idxend));
		}
	}

	bool HasData() const {
		return m_data.empty();
	}
	operator bool() const {
		return HasData();
	}
	
	string_view operator[](size_t idx) const noexcept {
		return m_lines[idx];
	}
	string_view At(size_t idx) const {
		return m_lines.at(idx);
	}
};

#endif // LIBARRIER_TEXTFILE_READER_HPP