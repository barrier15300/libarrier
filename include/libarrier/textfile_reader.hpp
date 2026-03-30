#ifndef LIBARRIER_TEXTFILE_READER_HPP
#define LIBARRIER_TEXTFILE_READER_HPP

#include <algorithm>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <ranges>
#include <string>
#include <string_view>
#include <vector>

namespace libarrier {

class TextfileReader {
	using string = std::string;
	using string_view = std::string_view;

	string m_data;
	std::vector<string_view> m_lines;

public:

	template<class T>
	class lines_view_base {
		T& m_reader;
		size_t m_begin;
		size_t m_count;

	public:

		using lines_view = lines_view_base<TextfileReader>;
		using const_lines_view = lines_view_base<const TextfileReader>;

		lines_view_base(T& reader) : m_reader(reader), m_count(m_reader.m_lines.size()) {}
		lines_view_base(T& reader, size_t begin, size_t count) :
			m_reader(reader),
			m_begin(std::min(begin, m_reader.m_lines.size())),
			m_count(std::min(count, max_size())) {}

		size_t max_size() const {
			return m_reader.m_lines.size() - m_begin;
		}

		string_view operator[](size_t idx) const {
			return m_reader.m_lines[m_begin + idx];
		}
		string_view at(size_t idx) const {
			return m_reader.m_lines.at(m_begin + idx);
		}

		auto begin() {
			return m_reader.m_lines.begin() + m_begin;
		}
		auto end() {
			return m_reader.m_lines.begin() + m_begin + m_count;
		}
		auto begin() const {
			return m_reader.m_lines.begin() + m_begin;
		}
		auto end() const {
			return m_reader.m_lines.begin() + m_begin + m_count;
		}
		auto cbegin() const {
			return m_reader.m_lines.cbegin() + m_begin;
		}
		auto cend() const {
			return m_reader.m_lines.cend() + m_begin + m_count;
		}
		auto rbegin() {
			return std::reverse_iterator(end());
		}
		auto rend() {
			return std::reverse_iterator(begin());
		}
		auto rbegin() const {
			return std::reverse_iterator(end());
		}
		auto rend() const {
			return std::reverse_iterator(begin());
		}
		auto crbegin() const {
			return std::reverse_iterator(end());
		}
		auto crend() const {
			return std::reverse_iterator(begin());
		}

		size_t size() const {
			return m_count;
		}
		bool empty() const {
			return size() == 0;
		}

		string_view subdata() const {
			size_t databegin = begin()->begin() - m_reader.data().begin();
			size_t dataend = rbegin()->end() - m_reader.data().begin();
			return string_view(m_reader.m_data).substr(databegin, dataend - databegin);
		}

		size_t lineof(size_t cursor) const {
			if (cursor >= subdata().size()) { return string_view::npos; }
			auto it = std::lower_bound(begin(), end(), subdata().substr(cursor, 1),
			                           [](const string_view& line, const string_view& target) {
				return line.end() < target.begin();
			});
			return std::distance(begin(), it);
		}

		template<size_t (string_view::*find_func)(string_view, size_t) const>
		class basic_exist_info {
			size_t m_idx;
			size_t m_elem;

		public:

			basic_exist_info(string_view data, string_view target, size_t offset = 0) {
				m_idx = (data.*find_func)(target, offset);
				m_elem = target.size();
			}

			explicit operator bool() const {
				return exist();
			}
			operator size_t() const {
				return idx();
			}

			bool exist() const {
				return m_idx != string_view::npos;
			}
			size_t idx() const {
				return m_idx;
			}
			size_t elem() const {
				return m_elem;
			}
			size_t next() const {
				return exist() ? m_idx + m_elem : string_view::npos;
			}
		};
		using exist_info = basic_exist_info<&string_view::find>;
		using exist_info_r = basic_exist_info<&string_view::rfind>;

		auto exist(string_view target, size_t offset = 0) const {
			return exist_info(subdata(), target, offset);
		}
		auto exist_r(string_view target, size_t offset = 0) const {
			return exist_info_r(subdata(), target, offset);
		}

		auto exist_all(string_view target) const {
			std::vector<exist_info> infos;
			size_t offset = 0;
			while (auto info = exist(target, offset)) {
				infos.push_back(info);
				offset = info.next();
			}
			return infos;
		}

		lines_view sublines(size_t start, size_t count = string_view::npos) {
			return lines_view(m_reader, m_begin + start, count);
		}
		const_lines_view sublines(size_t start, size_t count = string_view::npos) const {
			return const_lines_view(m_reader, m_begin + start, count);
		}
	};
	using lines_view = lines_view_base<TextfileReader>;
	using const_lines_view = lines_view_base<const TextfileReader>;

	using exist_info = const_lines_view::exist_info;
	using exist_info_r = const_lines_view::exist_info_r;

	friend lines_view;
	friend const_lines_view;

	TextfileReader() = default;
	TextfileReader(const string& path) {
		Read(path);
	}

	bool Read(const string& path) {
		std::ifstream ifs(path, std::ios::binary | std::ios::ate);

		if (!ifs.is_open()) { return false; }

		auto size = ifs.tellg();
		ifs.seekg(0, std::ios::beg);

		if (size < 0) { return false; }

		m_data.resize(static_cast<size_t>(size));
		ifs.read(m_data.data(), size);

		ifs.close();

		CreateIndex();

		return true;
	}
	void CreateIndex() {
		if (empty()) { return; }

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
		size_t idxbegin = ~delim.size() + 1;
		size_t idxend = 0;
		while (prev != string::npos) {
			prev = idxend + delim.size();
			idxbegin = prev;
			idxend = refdata.find(delim, prev);

			m_lines.push_back(refdata.substr(idxbegin, idxend));
		}
	}

	string_view data() {
		return m_data;
	}
	const string_view data() const {
		return m_data;
	}

	bool empty() const {
		return m_data.empty();
	}
	operator bool() const {
		return !empty();
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

	lines_view lines() {
		return lines_view(*this);
	}
	const_lines_view lines() const {
		return const_lines_view(*this);
	}

	size_t lineof(size_t cursor) const {
		return lines().lineof(cursor);
	}

	auto exist(string_view target, size_t offset = 0) const {
		return exist_info(m_data, target, offset);
	}
	auto exist_r(string_view target, size_t offset = 0) const {
		return exist_info_r(m_data, target, offset);
	}

	auto exist_all(string_view target) const {
		return lines().exist_all(target);
	}

	lines_view sublines(size_t start, size_t count = string_view::npos) {
		return lines_view(*this, start, count);
	}
	const_lines_view sublines(size_t start, size_t count = string_view::npos) const {
		return const_lines_view(*this, start, count);
	}
};

} // namespace libarrier

#endif // LIBARRIER_TEXTFILE_READER_HPP