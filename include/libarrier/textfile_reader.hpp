#ifndef LIBARRIER_TEXTFILE_READER_HPP
#define LIBARRIER_TEXTFILE_READER_HPP

#include <algorithm>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <memory>
#include <ranges>
#include <string>
#include <string_view>
#include <type_traits>
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
		T* m_reader = nullptr;
		size_t m_begin = 0;
		size_t m_count = 0;

		static constexpr bool is_const = std::is_const_v<T>;

	public:

		using lines_view = lines_view_base<TextfileReader>;
		using const_lines_view = lines_view_base<const TextfileReader>;

		using line_type = std::conditional_t<is_const, const string_view, string_view>;
		using lines_view_type = std::conditional_t<is_const, const_lines_view, lines_view>;

		lines_view_base() = delete;
		lines_view_base(const lines_view_base&) = default;
		lines_view_base(lines_view_base&&) = default;
		lines_view_base& operator=(const lines_view_base&) = default;
		lines_view_base& operator=(lines_view_base&&) = default;

		lines_view_base(T& reader) : m_reader(std::addressof(reader)), m_count(m_reader->m_lines.size()) {}
		lines_view_base(T& reader, size_t begin, size_t count) :
			m_reader(std::addressof(reader)),
			m_begin(std::min(begin, m_reader->m_lines.size())),
			m_count(std::min(count, max_size())) {}

		size_t max_size() const {
			return m_reader->m_lines.size() - m_begin;
		}

		line_type operator[](size_t idx)
			requires(!is_const)
		{
			return m_reader->m_lines[m_begin + idx];
		}
		line_type at(size_t idx)
			requires(!is_const)
		{
			return m_reader->m_lines.at(m_begin + idx);
		}
		line_type operator[](size_t idx) const
			requires(is_const)
		{
			return m_reader->m_lines[m_begin + idx];
		}
		line_type at(size_t idx) const
			requires(is_const)
		{
			return m_reader->m_lines.at(m_begin + idx);
		}

		auto begin()
			requires(!is_const)
		{
			return m_reader->m_lines.begin() + m_begin;
		}
		auto end()
			requires(!is_const)
		{
			return m_reader->m_lines.begin() + m_begin + m_count;
		}
		auto begin() const {
			return m_reader->m_lines.begin() + m_begin;
		}
		auto end() const {
			return m_reader->m_lines.begin() + m_begin + m_count;
		}
		auto cbegin() const {
			return m_reader->m_lines.cbegin() + m_begin;
		}
		auto cend() const {
			return m_reader->m_lines.cend() + m_begin + m_count;
		}
		auto rbegin()
			requires(!is_const)
		{
			return std::reverse_iterator(end());
		}
		auto rend()
			requires(!is_const)
		{
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

		line_type subdata() const {
			size_t databegin = std::to_address(begin()->begin()) - std::to_address(m_reader->data().begin());
			size_t dataend = std::to_address(rbegin()->end()) - std::to_address(m_reader->data().begin());
			return line_type(m_reader->m_data).substr(databegin, dataend - databegin);
		}

		size_t line_of(size_t cursor) const {
			auto sub = subdata();
			if (cursor >= sub.size()) { return line_type::npos; }
			auto it =
				std::lower_bound(begin(), end(), sub.substr(cursor, 1), [](const line_type& line, const line_type& target) {
				return std::to_address(line.end()) < std::to_address(target.begin());
			});
			return std::distance(begin(), it);
		}
		size_t cursor_of(size_t line) const {
			if (line >= size()) { return line_type::npos; }
			return std::to_address((*this)[line].begin()) - std::to_address(begin()->begin());
		}
		size_t line_column_of(size_t cursor) const {
			auto line = line_of(cursor);
			if (line == line_type::npos) { return line_type::npos; }
			return cursor - cursor_of(line);
		}

		template<size_t (line_type::*find_func)(line_type, size_t) const>
		class basic_exist_info {
			size_t m_idx;
			size_t m_elem;

		public:

			basic_exist_info(line_type data, line_type target, size_t offset = 0) :
				m_idx((data.*find_func)(target, offset)),
				m_elem(target.size()) {}

			explicit operator bool() const {
				return exist();
			}
			operator size_t() const {
				return idx();
			}

			bool exist() const {
				return m_idx != line_type::npos;
			}
			size_t idx() const {
				return m_idx;
			}
			size_t elem() const {
				return m_elem;
			}
			size_t next() const {
				return exist() ? m_idx + m_elem : line_type::npos;
			}
		};
		using exist_info = basic_exist_info<&line_type::find>;
		using exist_info_r = basic_exist_info<&line_type::rfind>;

		auto exist(line_type target, size_t offset = 0) const {
			return exist_info(subdata(), target, offset);
		}
		auto exist_r(line_type target, size_t offset = 0) const {
			return exist_info_r(subdata(), target, offset);
		}

		auto exist_all(line_type target) const {
			std::vector<exist_info> infos;
			size_t offset = 0;
			while (auto info = exist(target, offset)) {
				infos.push_back(info);
				offset = info.next();
			}
			return infos;
		}

		lines_view_type sublines(size_t start, size_t count = line_type::npos) const {
			return lines_view_type(*m_reader, m_begin + start, count);
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

		constexpr string_view endcodetype[] = {"\r\n", "\n", "\r", ""};
		size_type endlinecounts[] = {nsize, nsize, rsize, 0};

		m_lines.reserve(endlinecounts[endtypeindex] + 2);

		string_view delim = endcodetype[endtypeindex];
		string_view refdata = m_data;

		size_t idxbegin = 0;
		size_t idxend = refdata.find(delim);
		while (idxbegin != std::string_view::npos) {
			m_lines.push_back(refdata.substr(idxbegin, idxend - idxbegin));
			idxbegin = (idxend == std::string_view::npos) ? (idxend) : (idxend + delim.size());
			idxend = refdata.find(delim, idxbegin);
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

	size_t line_of(size_t cursor) const {
		return lines().line_of(cursor);
	}
	size_t cursor_of(size_t line) const {
		return lines().cursor_of(line);
	}
	size_t line_column_of(size_t cursor) const {
		return lines().line_column_of(cursor);
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