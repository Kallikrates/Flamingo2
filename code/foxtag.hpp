#ifndef FOXTAG_HPP
#define FOXTAG_HPP

#include <cstdint>
#include <fstream>
#include <memory>
#include <string>
#include <vector>

#include <QString>

namespace foxtag {

	class entry {
	public:
		entry() = default;
		entry(entry &&) = default;
		bool read_from(std::istream &);
		bool write_to(std::ostream &) const;
		QString const & get_path() {return path;}
		int8_t get_lev() {return lev;}
		std::vector<QString> & get_tags() {return tags;}
		void set_path(QString p) {path = p;}
		void set_lev(int8_t l) {lev = l;}
	protected:
		QString path;
		int8_t lev;
		std::vector<QString> tags;
	private:
		entry(entry const &) = delete;
		entry & operator=(entry const &) = delete;
	};

	class database {
	public:
		database() = default;
		database(database &&) = default;
		bool populate_from_file(std::string const &, std::string * error = nullptr);
		bool save_to_file(std::string const &, std::string * error = nullptr) const;
		void add_entry(std::shared_ptr<entry> &);
	protected:
		std::vector<std::shared_ptr<entry>> entries;
	private:
		database(database const &) = delete;
		database & operator=(database const &) = delete;
	};

}

#endif //FOXTAG_HPP
