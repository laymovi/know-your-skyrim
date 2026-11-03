#pragma once

#include <unordered_set>

// The files of the game the way the game finds them: a loose file in Data first, then the archives in the order the
// game loads them, the last loaded wins. Only the folders asked for are indexed, the scripts and the strings of the
// plugins. Archives of Skyrim Special Edition (version 105), their files packed with LZ4 or not. Made and read on one
// thread of the browser's, nothing here calls into the game
namespace GameArchives
{
	class Files
	{
	public:
		// a_archives are file names in Data in the order the game loads them, a_folders lower case "scripts"
		Files(const std::vector<std::string>& a_archives, std::initializer_list<std::string_view> a_folders);

		// the whole file by its path in Data, "scripts\\mq101script.pex" in any case. Nothing when no one has it or
		// it can't be read
		[[nodiscard]] std::optional<std::vector<std::byte>> Read(std::string_view a_path) const;

		// Each archive's stretch of the folder is read from the disk at once and held in memory until the files
		// go. Tens of thousands of small files read one by one in no order wait on the disk for each, 12 s for the
		// scripts of a big load order. Tells how many bytes are held
		std::size_t Hold(std::string_view a_folder);

		// how long the reads took, loose files, files of the archives and the unpacking of the packed ones
		struct Took
		{
			double loose = 0.0;
			double archived = 0.0;
			double unpacked = 0.0;
			std::size_t looseFiles = 0;
			std::size_t archivedFiles = 0;
			std::size_t unpackedFiles = 0;
		};
		[[nodiscard]] const Took& Taken() const { return took; }

		[[nodiscard]] std::size_t Archived() const { return entries.size(); }
		[[nodiscard]] std::size_t Loose() const { return loose.size(); }
		[[nodiscard]] std::size_t Archives() const { return archives.size(); }

	private:
		struct Entry
		{
			std::uint32_t archive = 0;
			std::uint32_t offset = 0;
			std::uint32_t size = 0;
			bool packed = false;
		};

		// an archive mapped into memory for as long as the files last, a read is a copy out of it
		class Archive
		{
		public:
			explicit Archive(std::filesystem::path a_path);
			~Archive();
			Archive(Archive&& a_other) noexcept;
			Archive(const Archive&) = delete;
			Archive& operator=(const Archive&) = delete;
			Archive& operator=(Archive&&) = delete;

			[[nodiscard]] std::span<const std::byte> Bytes() const { return { view, size }; }
			// a_size bytes from a_from, out of the stretch held in memory when it has them, empty past the end
			[[nodiscard]] std::span<const std::byte> Part(std::size_t a_from, std::size_t a_size) const;
			// the stretch read from the disk in one go, false when it can't be
			bool Hold(std::size_t a_from, std::size_t a_to);
			void Close();

			std::filesystem::path path;
			bool namesInData = false;
			// it holds files of a folder asked for, it stays mapped
			bool wanted = false;

		private:
			void* file = nullptr;
			void* mapping = nullptr;
			const std::byte* view = nullptr;
			std::size_t size = 0;
			std::size_t heldFrom = 0;
			std::vector<std::byte> held;
		};

		// where the files of one folder lie in one archive, from the first to the end of the last
		struct Stretch
		{
			std::uint32_t archive = 0;
			std::size_t from = 0;
			std::size_t to = 0;
		};

		void Index(std::uint32_t a_archive, std::span<const std::string_view> a_folders);

		std::vector<Archive> archives;
		std::unordered_map<std::string, Entry> entries;
		std::unordered_set<std::string> loose;
		std::unordered_map<std::string, std::vector<Stretch>> stretches;
		mutable Took took;
	};

	// lower case with back slashes, the way the index keeps paths
	std::string Key(std::string_view a_path);

	// a path in the system's code page, the way the game's files name it. Letters that page hasn't got make string()
	// throw, such a path comes in UTF-8
	std::string Narrow(const std::filesystem::path& a_path);

	// a frame of LZ4 into a_out, false for anything that isn't one
	bool Unpack(std::span<const std::byte> a_frame, std::vector<std::byte>& a_out);

	// the archives the game loads, asked on the game's thread: the ones its ini lists, then one for each plugin
	std::vector<std::string> LoadOrderArchives();

	// the language of the game's strings files, "ENGLISH"
	std::string Language();
}
