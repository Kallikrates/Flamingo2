#include "foxtag.hpp"

//Entry

bool foxtag::entry::read_from(std::istream & s) {
	if (s.rdstate() != std::ios::goodbit) return false;
	char ci;
	std::vector<char> pathv;
	do {
		s.read(&ci, 1);
		if (s.rdstate() != std::ios::goodbit) return false;
		pathv.push_back(ci);
	} while (ci != '\0');
	path = QString::fromUtf8(pathv.data());
	s.read(reinterpret_cast<char *>(&lev), sizeof(lev));
	if (s.rdstate() != std::ios::goodbit) return false;
	uint16_t tagnum;
	s.read(reinterpret_cast<char *>(&tagnum), sizeof(tagnum));
	if (s.rdstate() != std::ios::goodbit) return false;
	std::vector<char> tagv;
	tags.reserve(tagnum);
	for (uint16_t ct = 0; ct < tagnum; ct++) {
		tagv.clear();
		do {
			s.read(&ci, 1);
			if (s.rdstate() != std::ios::goodbit) return false;
			tagv.push_back(ci);
		} while (ci != '\0');
		tags.push_back(QString::fromUtf8(tagv.data()));
	}
	return true;
}

bool foxtag::entry::write_to(std::ostream & s) const {
	if (s.rdstate() != std::ios::goodbit) return false;
	QByteArray pathl = path.toUtf8();
	s.write(pathl.data(), pathl.length());
	s.write("\0", 1);
	if (s.rdstate() != std::ios::goodbit) return false;
	s.write(reinterpret_cast<char const *>(&lev), sizeof(lev));
	if (s.rdstate() != std::ios::goodbit) return false;
	uint16_t taglen = tags.size();
	s.write(reinterpret_cast<char const *>(&taglen), sizeof(taglen));
	if (s.rdstate() != std::ios::goodbit) return false;
	for (QString const & tag : tags) {
		QByteArray tagl = tag.toLatin1();
		s.write(tagl.data(), tagl.length());
		s.write("\0", 1);
		if (s.rdstate() != std::ios::goodbit) return false;
	}
	return true;
}

//Database

bool foxtag::database::save_to_file(std::string const & path, std::string * error) const {
	std::ofstream o(path, std::ios::binary | std::ios::out | std::ios::trunc);
	if (o.rdstate() != std::ios::goodbit) {
		if (error) *error = "could not open/create file for writing";
		return false;
	}
	uint32_t ecount = entries.size();
	o.write("FOXTAG", 6);
	o.write(reinterpret_cast<char const *>(&ecount), sizeof(ecount));
	if (o.rdstate() != std::ios::goodbit) {
		if (error) *error = "failed to write header";
		return false;
	}
	for (std::shared_ptr<entry> const & entry : entries) {
		if (!entry->write_to(o)) {
			if (error) *error = "file became unwritable midway through writing for unknown reason";
			return false;
		}
	}
	return true;
}

bool foxtag::database::populate_from_file(std::string const & path, std::string * error) {
	return false;
}

void foxtag::database::add_entry(std::shared_ptr<entry> & e) {
	entries.push_back(e);
}
