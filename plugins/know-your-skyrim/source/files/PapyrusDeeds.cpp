#include "PCH.h"

#include "PapyrusDeeds.h"

namespace PapyrusDeeds
{
	namespace
	{
		constexpr std::uint32_t kMagic = 0xFA57C0DE;
		// a variable holds this many values at most, a loop of assignments can't grow it further
		constexpr std::size_t kMostValues = 6;

		enum Op : std::uint8_t
		{
			kAssign = 13,
			kCast = 14,
			kCallMethod = 23,
			kCallParent = 24,
			kCallStatic = 25,
			kReturn = 26,
			kPropertyGet = 28,
			kLastOp = 35
		};

		// how many arguments each operation takes before the ones a call adds
		constexpr std::array<std::uint8_t, kLastOp + 1> kArguments{ 0, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 1, 2, 2, 3, 2, 3, 1, 3, 3, 3, 2, 2, 3, 3, 4, 4 };

		enum class Value : std::uint8_t
		{
			kNone,
			kIdentifier,
			kString,
			kInteger,
			kFloat,
			kBool
		};

		struct Argument
		{
			Value type = Value::kNone;
			std::uint16_t text = 0;
			std::int32_t integer = 0;
			float real = 0.0F;
		};

		// the file is big endian, every read checks it stays inside
		class Reader
		{
		public:
			explicit Reader(std::span<const std::byte> a_data) :
				data(a_data) {}

			[[nodiscard]] bool Good() const { return good; }

			template <class T>
			T Take()
			{
				T value{};
				if (at + sizeof(T) > data.size()) {
					good = false;
					return value;
				}
				std::array<std::byte, sizeof(T)> bytes{};
				for (std::size_t i = 0; i < sizeof(T); ++i) {
					bytes[i] = data[at + sizeof(T) - 1 - i];
				}
				std::memcpy(&value, bytes.data(), sizeof(T));
				at += sizeof(T);
				return value;
			}

			std::string Text()
			{
				const auto size = Take<std::uint16_t>();
				if (!good || at + size > data.size()) {
					good = false;
					return {};
				}
				std::string text(reinterpret_cast<const char*>(data.data() + at), size);
				at += size;
				return text;
			}

			void Skip(std::size_t a_bytes)
			{
				at += a_bytes;
				good = good && at <= data.size();
			}

			void Spoil() { good = false; }

		private:
			std::span<const std::byte> data;
			std::size_t at = 0;
			bool good = true;
		};

		std::string Lower(std::string_view a_text)
		{
			std::string lower(a_text);
			for (auto& letter : lower) {
				letter = static_cast<char>(std::tolower(static_cast<unsigned char>(letter)));
			}
			return lower;
		}

		Argument TakeArgument(Reader& a_reader)
		{
			Argument argument;
			argument.type = static_cast<Value>(a_reader.Take<std::uint8_t>());
			switch (argument.type) {
			case Value::kIdentifier:
			case Value::kString:
				argument.text = a_reader.Take<std::uint16_t>();
				break;
			case Value::kInteger:
				argument.integer = a_reader.Take<std::int32_t>();
				break;
			case Value::kFloat:
				argument.real = a_reader.Take<float>();
				break;
			case Value::kBool:
				argument.integer = a_reader.Take<std::uint8_t>();
				break;
			default:
				break;
			}
			return argument;
		}

		class Builder
		{
		public:
			// The names of the script's table in lower case, and for each the first of the table with the same letters:
			// a variable is found by its number, a fragment's code reads thousands of them
			Builder(Script& a_script, const std::vector<std::string>& a_strings) :
				script(a_script)
			{
				std::unordered_map<std::string, std::uint16_t> first;
				for (std::size_t i = 0; i < a_strings.size(); ++i) {
					lowered.push_back(Lower(a_strings[i]));
					canonical.push_back(first.try_emplace(lowered.back(), static_cast<std::uint16_t>(i)).first->second);
				}
			}

			// the variable that holds an auto property, by its name's number in the table
			void Auto(std::uint16_t a_variable, std::string a_property)
			{
				if (a_variable < canonical.size()) {
					autos[canonical[a_variable]] = std::move(a_property);
				}
			}

			void Function(Reader& a_reader, std::string a_name)
			{
				const auto function = static_cast<std::uint32_t>(script.functions.size());
				script.functions.push_back(Lower(a_name));
				a_reader.Skip(2 + 2 + 4 + 1);
				const auto params = a_reader.Take<std::uint16_t>();
				a_reader.Skip(static_cast<std::size_t>(params) * 4);
				const auto locals = a_reader.Take<std::uint16_t>();
				a_reader.Skip(static_cast<std::size_t>(locals) * 4);
				const auto count = a_reader.Take<std::uint16_t>();
				values.clear();
				std::vector<Argument> arguments;
				for (std::uint16_t i = 0; i < count && a_reader.Good(); ++i) {
					const auto op = a_reader.Take<std::uint8_t>();
					if (op > kLastOp) {
						a_reader.Spoil();
						return;
					}
					arguments.clear();
					for (std::uint8_t a = 0; a < kArguments[op]; ++a) {
						arguments.push_back(TakeArgument(a_reader));
					}
					if (op == kCallMethod || op == kCallParent || op == kCallStatic) {
						const auto more = TakeArgument(a_reader);
						for (std::int32_t a = 0; a < more.integer && a_reader.Good(); ++a) {
							arguments.push_back(TakeArgument(a_reader));
						}
					}
					Step(op, arguments, function);
				}
			}

		private:
			// one symbol of a kind, number, inner symbol and name in the script, found by a hash of them
			std::int32_t Intern(Symbol a_symbol)
			{
				const auto hash = std::hash<std::string>{}(a_symbol.name) ^ (static_cast<std::size_t>(a_symbol.kind) << 56) ^
				                  (static_cast<std::size_t>(static_cast<std::uint32_t>(a_symbol.number)) << 20) ^ static_cast<std::size_t>(a_symbol.of + 1);
				auto& same = interned[hash];
				for (const auto at : same) {
					const auto& had = script.symbols[at];
					if (had.kind == a_symbol.kind && had.number == a_symbol.number && had.of == a_symbol.of && had.name == a_symbol.name) {
						return at;
					}
				}
				const auto at = static_cast<std::int32_t>(script.symbols.size());
				script.symbols.push_back(std::move(a_symbol));
				same.push_back(at);
				return at;
			}

			[[nodiscard]] std::string_view Word(std::uint16_t a_index) const
			{
				return a_index < lowered.size() ? std::string_view(lowered[a_index]) : std::string_view();
			}

			[[nodiscard]] std::uint16_t Same(std::uint16_t a_index) const
			{
				return a_index < canonical.size() ? canonical[a_index] : a_index;
			}

			std::vector<std::int32_t> Evaluate(const Argument& a_argument)
			{
				switch (a_argument.type) {
				case Value::kIdentifier:
					{
						if (Word(a_argument.text) == "self") {
							return { Intern({ Symbol::Kind::kSelf }) };
						}
						const auto same = Same(a_argument.text);
						if (const auto property = autos.find(same); property != autos.end()) {
							return { Intern({ Symbol::Kind::kProperty, 0, property->second }) };
						}
						const auto known = values.find(same);
						return known != values.end() ? known->second : std::vector<std::int32_t>{};
					}
				case Value::kString:
					return { Intern({ Symbol::Kind::kText, 0, std::string(Word(a_argument.text)) }) };
				case Value::kInteger:
				case Value::kBool:
					return { Intern({ Symbol::Kind::kNumber, a_argument.integer }) };
				case Value::kFloat:
					return { Intern({ Symbol::Kind::kNumber, static_cast<std::int32_t>(a_argument.real) }) };
				default:
					return {};
				}
			}

			// A value written over a variable replaces what it held, because the code of a fragment is straight and a
			// temporary is filled right before the call that takes it
			void Put(const Argument& a_into, std::vector<std::int32_t> a_values)
			{
				if (a_into.type != Value::kIdentifier) {
					return;
				}
				if (a_values.size() > kMostValues) {
					a_values.resize(kMostValues);
				}
				values[Same(a_into.text)] = std::move(a_values);
			}

			std::vector<std::int32_t> Wrapped(Symbol::Kind a_kind, const std::vector<std::int32_t>& a_of, std::string a_name = {})
			{
				std::vector<std::int32_t> wrapped;
				for (const auto of : a_of) {
					wrapped.push_back(Intern({ a_kind, 0, a_name, of }));
				}
				return wrapped;
			}

			void Record(Deed a_deed, std::uint32_t a_function, std::vector<std::int32_t> a_target, std::span<const Argument> a_arguments)
			{
				Call call{ a_deed, a_function, std::move(a_target) };
				for (const auto& argument : a_arguments) {
					call.arguments.push_back(Evaluate(argument));
				}
				script.calls.push_back(std::move(call));
			}

			void Step(std::uint8_t a_op, const std::vector<Argument>& a_arguments, std::uint32_t a_function)
			{
				switch (a_op) {
				case kAssign:
				case kCast:
					Put(a_arguments[0], Evaluate(a_arguments[1]));
					break;
				case kPropertyGet:
					Put(a_arguments[2], Wrapped(Symbol::Kind::kPropertyOf, Evaluate(a_arguments[1]), std::string(Word(a_arguments[0].text))));
					break;
				case kCallStatic:
					Static(a_arguments, a_function);
					break;
				case kCallMethod:
					Method(a_arguments, a_function);
					break;
				default:
					break;
				}
			}

			void Static(const std::vector<Argument>& a_arguments, std::uint32_t a_function)
			{
				const auto type = Word(a_arguments[0].text);
				const auto name = Word(a_arguments[1].text);
				const std::span<const Argument> rest(a_arguments.begin() + 3, a_arguments.end());
				if (type == "game" && name == "getplayer") {
					Put(a_arguments[2], { Intern({ Symbol::Kind::kPlayer }) });
				} else if (type == "quest" && name == "getquest" && !rest.empty() && rest[0].type == Value::kString) {
					Put(a_arguments[2], { Intern({ Symbol::Kind::kQuestNamed, 0, std::string(Word(rest[0].text)) }) });
				} else if (type == "game" && name == "getformfromfile" && rest.size() > 1 && rest[0].type == Value::kInteger && rest[1].type == Value::kString) {
					Put(a_arguments[2], { Intern({ Symbol::Kind::kFormFromFile, rest[0].integer, std::string(Word(rest[1].text)) }) });
				} else if (type == "game") {
					static const std::unordered_map<std::string_view, Deed> deeds{ { "teachword", Deed::kTeachWord }, { "unlockword", Deed::kTeachWord },
						{ "incrementskill", Deed::kIncrementSkill }, { "incrementskillby", Deed::kIncrementSkill }, { "advanceskill", Deed::kAdvanceSkill },
						{ "addperkpoints", Deed::kAddPerkPoints } };
					if (const auto deed = deeds.find(name); deed != deeds.end()) {
						Record(deed->second, a_function, {}, rest);
					}
				}
			}

			void Method(const std::vector<Argument>& a_arguments, std::uint32_t a_function)
			{
				const auto name = Word(a_arguments[0].text);
				auto target = Evaluate(a_arguments[1]);
				const std::span<const Argument> rest(a_arguments.begin() + 3, a_arguments.end());
				if (name == "getowningquest") {
					Put(a_arguments[2], { Intern({ Symbol::Kind::kOwnQuest }) });
					return;
				}
				if (name == "getreference" || name == "getref" || name == "getactorreference" || name == "getactorref") {
					Put(a_arguments[2], Wrapped(Symbol::Kind::kAliasRef, target));
					return;
				}
				if (name == "getvalue" || name == "getvalueint") {
					Put(a_arguments[2], Wrapped(Symbol::Kind::kValueOf, target));
					return;
				}
				static const std::unordered_map<std::string_view, Deed> deeds{ { "additem", Deed::kAddItem }, { "removeitem", Deed::kRemoveItem },
					{ "addspell", Deed::kAddSpell }, { "addperk", Deed::kAddPerk }, { "addshout", Deed::kAddShout }, { "modactorvalue", Deed::kModActorValue },
					{ "setstage", Deed::kSetStage }, { "setcurrentstageid", Deed::kSetStage }, { "start", Deed::kStart }, { "reset", Deed::kStart },
					{ "stop", Deed::kStop }, { "completequest", Deed::kComplete }, { "completeallobjectives", Deed::kComplete } };
				// a giver of RemoveItem into the player is often the speaker of a line, a parameter that holds nothing here
				if (const auto deed = deeds.find(name); deed != deeds.end() && (!target.empty() || deed->second == Deed::kRemoveItem)) {
					Record(deed->second, a_function, std::move(target), rest);
				}
			}

			Script& script;
			std::vector<std::string> lowered;
			std::vector<std::uint16_t> canonical;
			std::unordered_map<std::uint16_t, std::string> autos;
			std::unordered_map<std::uint16_t, std::vector<std::int32_t>> values;
			std::unordered_map<std::size_t, std::vector<std::int32_t>> interned;
		};

		void SkipFunction(Reader& a_reader)
		{
			a_reader.Skip(2 + 2 + 4 + 1);
			a_reader.Skip(static_cast<std::size_t>(a_reader.Take<std::uint16_t>()) * 4);
			a_reader.Skip(static_cast<std::size_t>(a_reader.Take<std::uint16_t>()) * 4);
			const auto count = a_reader.Take<std::uint16_t>();
			for (std::uint16_t i = 0; i < count && a_reader.Good(); ++i) {
				const auto op = a_reader.Take<std::uint8_t>();
				if (op > kLastOp) {
					a_reader.Spoil();
					return;
				}
				for (std::uint8_t a = 0; a < kArguments[op]; ++a) {
					TakeArgument(a_reader);
				}
				if (op == kCallMethod || op == kCallParent || op == kCallStatic) {
					const auto more = TakeArgument(a_reader);
					for (std::int32_t a = 0; a < more.integer && a_reader.Good(); ++a) {
						TakeArgument(a_reader);
					}
				}
			}
		}

		// The value a read only property gives, "Int Property RewardAmount = 500 AutoReadOnly": its handler is one return
		// of a number. Any other handler gives none
		std::optional<std::int32_t> ReturnedNumber(Reader& a_reader)
		{
			a_reader.Skip(2 + 2 + 4 + 1);
			a_reader.Skip(static_cast<std::size_t>(a_reader.Take<std::uint16_t>()) * 4);
			a_reader.Skip(static_cast<std::size_t>(a_reader.Take<std::uint16_t>()) * 4);
			const auto count = a_reader.Take<std::uint16_t>();
			std::optional<std::int32_t> returned;
			for (std::uint16_t i = 0; i < count && a_reader.Good(); ++i) {
				const auto op = a_reader.Take<std::uint8_t>();
				if (op > kLastOp) {
					a_reader.Spoil();
					return std::nullopt;
				}
				std::vector<Argument> arguments;
				for (std::uint8_t a = 0; a < kArguments[op]; ++a) {
					arguments.push_back(TakeArgument(a_reader));
				}
				if (op == kCallMethod || op == kCallParent || op == kCallStatic) {
					const auto more = TakeArgument(a_reader);
					for (std::int32_t a = 0; a < more.integer && a_reader.Good(); ++a) {
						TakeArgument(a_reader);
					}
				}
				const bool number = !arguments.empty() && (arguments[0].type == Value::kInteger || arguments[0].type == Value::kFloat);
				returned = count == 1 && op == kReturn && number ? std::optional<std::int32_t>(arguments[0].type == Value::kInteger ? arguments[0].integer : static_cast<std::int32_t>(arguments[0].real)) : std::nullopt;
			}
			return returned;
		}
	}

	std::optional<Script> Read(std::span<const std::byte> a_pex)
	{
		Reader reader(a_pex);
		if (reader.Take<std::uint32_t>() != kMagic) {
			return std::nullopt;
		}
		reader.Skip(1 + 1 + 2 + 8);
		for (int i = 0; i < 3; ++i) {
			reader.Text();
		}
		std::vector<std::string> strings(reader.Take<std::uint16_t>());
		for (auto& text : strings) {
			text = reader.Text();
		}
		if (reader.Take<std::uint8_t>() != 0) {
			reader.Skip(8);
			const auto functions = reader.Take<std::uint16_t>();
			for (std::uint16_t i = 0; i < functions && reader.Good(); ++i) {
				reader.Skip(2 + 2 + 2 + 1);
				reader.Skip(static_cast<std::size_t>(reader.Take<std::uint16_t>()) * 2);
			}
		}
		reader.Skip(static_cast<std::size_t>(reader.Take<std::uint16_t>()) * 3);
		const auto word = [&](std::uint16_t a_index) { return a_index < strings.size() ? std::string_view(strings[a_index]) : std::string_view(); };

		Script script;
		Builder builder(script, strings);
		const auto objects = reader.Take<std::uint16_t>();
		for (std::uint16_t o = 0; o < objects && reader.Good(); ++o) {
			reader.Skip(2 + 4);
			script.parent = Lower(word(reader.Take<std::uint16_t>()));
			reader.Skip(2 + 4 + 2);
			// the variables come before the properties that name them, their first values are kept by name for now
			std::unordered_map<std::string, std::int32_t> firstValues;
			const auto variables = reader.Take<std::uint16_t>();
			for (std::uint16_t v = 0; v < variables && reader.Good(); ++v) {
				const auto name = Lower(word(reader.Take<std::uint16_t>()));
				reader.Skip(2 + 4);
				const auto value = TakeArgument(reader);
				if (value.type == Value::kInteger || value.type == Value::kBool) {
					firstValues[name] = value.integer;
				} else if (value.type == Value::kFloat) {
					firstValues[name] = static_cast<std::int32_t>(value.real);
				}
			}
			const auto properties = reader.Take<std::uint16_t>();
			for (std::uint16_t p = 0; p < properties && reader.Good(); ++p) {
				const auto name = Lower(word(reader.Take<std::uint16_t>()));
				reader.Skip(2 + 2 + 4);
				const auto flags = reader.Take<std::uint8_t>();
				if (flags & 0x4) {
					const auto index = reader.Take<std::uint16_t>();
					const auto variable = Lower(word(index));
					builder.Auto(index, name);
					if (const auto first = firstValues.find(variable); first != firstValues.end()) {
						script.numbers[name] = first->second;
					}
					continue;
				}
				if (flags & 0x1) {
					if (const auto returned = ReturnedNumber(reader)) {
						script.numbers[name] = *returned;
					}
				}
				if (flags & 0x2) {
					SkipFunction(reader);
				}
			}
			const auto states = reader.Take<std::uint16_t>();
			for (std::uint16_t s = 0; s < states && reader.Good(); ++s) {
				reader.Skip(2);
				const auto functions = reader.Take<std::uint16_t>();
				for (std::uint16_t f = 0; f < functions && reader.Good(); ++f) {
					const auto name = word(reader.Take<std::uint16_t>());
					builder.Function(reader, std::string(name));
				}
			}
		}
		if (!reader.Good()) {
			return std::nullopt;
		}
		return script;
	}
}
