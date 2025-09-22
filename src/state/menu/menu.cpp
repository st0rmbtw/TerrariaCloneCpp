#include <SGE/types/order.hpp>
#include <SGE/types/sprite.hpp>
#include <SGE/engine.hpp>
#include <SGE/types/color.hpp>
#include <SGE/input.hpp>
#include <SGE/time/time.hpp>
#include <SGE/types/anchor.hpp>
#include <SGE/math/quat.hpp>
#include <SGE/utils/text.hpp>
#include <SGE/utils/utf8.hpp>
#include <SGE/assert.hpp>

#include <glm/trigonometric.hpp>

#include "../../ui/ui.hpp"
#include "../../assets.hpp"
#include "../../app.hpp"

#include "../common.hpp"
#include "../ingame.hpp"

#include "menu.hpp"
#include "widgets.hpp"

static constexpr float LOGO_ANIM_MIN_SCALE = 0.9f;
static constexpr float LOGO_ANIM_MAX_SCALE = 1.1f;
static constexpr float LOGO_ANIM_MIN_ROTATION = -5.0f;
static constexpr float LOGO_ANIM_MAX_ROTATION = 5.0f;

static std::array WORLD_NAME_ADJECTIVES = std::to_array<std::string_view>({
    "Abandoned", "Abhorrent", "Abusive", "Adorable", "Adventurous", "Ageless", "Aggravating", "Aggressive", "Agile", "Agreeable", "Alert", "Alien", "Alive", "Alleged", "Aloof", "Amber", "Amethyst", "Amusing", "Ancient", "Angelic", "Angry", "Annoyed", "Annoying", "Anxious", "Apple", "Archaic", "Ardent", "Arrogant", "Ashamed", "Attractive", "Aunt Becky's", "Average", "Awful", "Awkward", "Babbling", "Bad", "Baleful", "Ballin'", "Bashful", "Basic", "Beautiful", "Best", "Bewildered", "Big", "Bitter", "Bizarre", "Black", "Blackish", "Bleeding", "Blistering", "Blocky", "Bloody", "Blooming", "Bloopy", "Blue", "Blushful", "Blushing", "Bob Saget's", "Bodacious", "Boorish", "Bored", "Boundless", "Brainy", "Brash", "Brave", "Breakable", "Breakless", "Breathtaking", "Bright", "Brilliant", "Broken", "Brown", "Brutal", "Budding", "Burning", "Busy", "Calm", "Canadian", "Canceled", "Careful", "Cautious", "Celestial", "Chad's", "Charming", "Chartreuse", "Cheap", "Cheerful", "Cherry", "Chicken", "Chippy's", "Clean", "Clear", "Clearable", "Clever", "Cleverish", "Cloudy", "Clumsy", "Cold", "Colorful", "Combative", "Comfortable", "Comical", "Common", "Complacent", "Compulsive", "Concerned", "Condemnable", "Condemned", "Confident", "Confusable", "Confused", "Constant", "Cooked", "Cooperative", "Corpulent", "Corrupt", "Corrupted", "Cosmic", "Courageous", "Cracked", "Cranky", "Crass", "Crazy", "Creepy", "Cringe", "Crowded", "Crude", "Cruel", "Cuddly", "Curious", "Cursed", "Cute", "Cyan", "Daft", "Damaged", "Dangerous", "Dank", "Dapper", "Dark", "Daunting", "Dead", "Dead Man's", "Decaying", "Deceased", "Defeated", "Defiant", "Degenerative", "Delightful", "Demented", "Demonic", "Dense", "Depressed", "Deranged", "Desolate", "Desperate", "Determined", "Devil's", "Devious", "Diamond", "Different", "Difficult", "Dire", "Dirty", "Disappointing", "Discarded", "Disgusting", "Disloyal", "Disruptive", "Distant", "Distinct", "Distorted", "Distressing", "Disturbed", "Divine", "Dizzy", "Docile", "Dope", "Doubtful", "Drab", "Draconian", "Dreadful", "Dreamy", "Dripping", "Dry", "Dubious", "Dull", "Dumb", "Eager", "Easy", "Eerie", "Egg Faced", "Elated", "Elegant", "Embarrassed", "Embarrassing", "Emerald", "Empty", "Enchanting", "Encouraging", "Enduring", "Energetic", "Enthusiastic", "Envious", "Erratic", "Eternal", "Euphoric", "Everlasting", "Evil", "Exalted", "Excellent", "Excited", "Exiled", "Existential", "Exotic", "Expensive", "Extinct", "Extra", "Extraordinary", "Extravagant", "Exuberant", "Fabulous", "Fair", "Faithful", "Fallen", "Famous", "Fancy", "Fantastic", "Far", "Faraway", "Fearful", "Fearsome", "Feckless", "Feral", "Fertile", "Festering", "Fetid", "Fierce", "Filthy", "Fine", "Firm", "Flaky", "Flaming", "Flat", "Flatulent", "Fleek", "Flexing", "Flowering", "Flowing", "Fluffy", "Foolhardy", "Foolish", "Forceful", "Foreign", "Forgiving", "Forgotten", "Forsaken", "Fortified", "Foul", "Fragile", "Frail", "Frantic", "Fraudulent", "Fresh", "Friendly", "Frightened", "Frightening", "Frisky", "Fruitful", "Funny", "Furious", "Gangrenous", "Gentle", "Ghosted", "Gifted", "Glamorous", "Gleaming", "Gleamless", "Gloomy", "Glorious", "Glowing", "Glubbed Up", "Goblin", "Godly", "Golden", "Good", "Gorgeous", "Graceful", "Grand", "Grassy", "Gray", "Greasy", "Great", "Greedy", "Green", "Griefing", "Grieving", "Grizzly", "Grotesque", "Grouchy", "Growing", "Gruesome", "Grumpy", "Guide's", "Guilty", "Gutless", "Hallowed", "Handsome", "Happy", "Happy Little", "Hardcore", "Harsh", "Hateful", "Hazardous", "Healthy", "Heartless", "Heavenly", "Heinous", "Helpful", "Helpless", "Hesitant", "Hidden", "Hideous", "Highkey", "Hilarious", "Holy", "Homeless", "Homely", "Horrible", "Horrific", "Horrifying", "Hot", "Hungry", "Hurt", "Hurtable", "Hurtful", "Hybrid", "Hyper", "Hysterical", "Ignorant", "Illicit", "Illusive", "Imaginary", "Immortal", "Incompetent", "Inconvenient", "Indecisive", "Indifferent", "Infected", "Inferior", "Infinite", "Insecure", "Insidious", "Insolent", "Intense", "Irresponsible", "Irritating", "Isolated", "Itchy", "Jade", "Jealous", "Jiggly", "Jittery", "Jolly", "Joyous", "Judgmental", "Karen's", "Keen", "Kind", "Kooky", "Lagging", "Large", "Lasting", "Lavender", "Lavish", "Lazy", "Leafy", "Legendary", "Lemon", "Light", "Light's", "Lightful", "Lime", "Little Known", "Lively", "Lonely", "Long", "Lost", "Lousy", "Lovely", "Lowkey", "Loyal", "Lucky", "Luminous", "Lumpy", "Lush", "Mad", "Magical", "Magnetic", "Magnificent", "Marshmallow", "Marvelous", "Massive", "Mathematical", "Menacing", "Merciless", "Mischievous", "Miserable", "Misty", "Modern", "Modular", "Moist", "Moldy", "Moon Lord's", "Moonlit", "Mopey", "Motionless", "Mourning", "Muddy", "Multitalented", "Murderous", "Murky", "Mushy", "Mysterious", "Mythical", "Naive", "Naked", "Nameless", "Nasty", "Natchy", "Natural", "Naughty", "Nauseating", "Navy", "Neglected", "Nervous", "New", "Nice", "Night's", "Nimble", "Non-Fungible", "Noxious", "Nude", "Nutty", "Obedient", "Obnoxious", "Obsessive", "Obstructed", "Obtuse", "Odd", "Offended", "Offensive", "Old", "Old-fashioned", "Olive", "Open", "Optimistic", "Orange", "Outraged", "Outrageous", "Outstanding", "Overcrowded", "Overeasy", "Overjealous", "Overjoyous", "Overrun", "Painter's", "Pale", "Panicky", "Partying", "Patronizing", "Peaceful", "Peach", "Perfect", "Perilous", "Perpetual", "Persistent", "Petrified", "Pine", "Pink", "Placid", "Plain", "Planking", "Pleasant", "Pleasing", "Pointy", "Poised", "Poison", "Poor", "Posturing", "Powerful", "Precious", "Prickly", "Prismatic", "Proud", "Pumpkin", "Purified", "Purple", "Putrid", "Puzzled", "Quaint", "Quick", "Quiet", "Quirky", "Rancid", "Rank", "Raunchy", "Raw", "Real", "Reckless", "Red", "Red's", "Reeking", "Rejected", "Relentless", "Relievable", "Relieved", "Remote", "Rent Free", "Repugnant", "Repulsive", "Resentful", "Restful", "Revolting", "Rich", "Ridiculous", "Risky", "Rotten", "Rotting", "Round", "Royal", "Ruby", "Rude", "Ruthless", "Sacred", "Sad", "Sadistic", "Safe", "Sage", "Sallow", "Salty", "Sandy", "Sapphire", "Saucy", "Savage", "Scandalous", "Scary", "Searing", "Seasick", "Sedated", "Selfish", "Senile", "Senseless", "Sensitive", "Serene", "Serious", "Shabby", "Shameful", "Sharp", "Shiny", "Shocking", "Shoddy", "Shy", "Significant", "Silenced", "Silly", "Silver", "Simple", "Skeleton", "Skeletron's", "Skronked Up", "Sleazy", "Sleepy", "Slow", "Sluggish", "Smelly", "Smiling", "Smoggy", "Smol", "Smooth", "Smouldering", "Solar", "Solid", "Sore", "Sour", "Sparkling", "Spastic", "Spicy", "Spiritual", "Splendid", "Spoiled", "Spooky", "Spotless", "Stable", "Stalwart", "Staunch", "Steaming", "Sticky", "Stiff", "Still", "Stinky", "Stormy", "Strange", "Strong", "Stupid", "Subaverage", "Subpar", "Successful", "Super", "Superb", "Superior", "Supernatural", "Supreme", "Sus", "Sweaty", "Tainted", "Talented", "Tame", "Tasty", "Teal", "Tenacious", "Tender", "Tense", "Terrible", "Terrified", "Thankful", "Thanos'", "Thick", "Thirsty", "Thoughtful", "Thoughtless", "Timeless", "Tiny", "Tired", "Topaz", "Tough", "Tranquil", "Trashy", "Traveling", "Tropical", "Troubled", "Trusting", "Ugliest", "Ugly", "Unacceptable", "Unbreakable", "Unbroken", "Uncanny", "Uncharming", "Uncharted", "Uncombative", "Uncooperative", "Undead", "Undetermined", "Undiscovered", "Undying", "Unending", "Unexplored", "Unforgivable", "Unhappy", "Unhurt", "Unhurting", "Uninterested", "Unknown", "Unlawful", "Unpleasant", "Unreal", "Unruly", "Unsightly", "Unsure", "Untalented", "Untamed", "Unusual", "Unwicked", "Unworried", "Upset", "Upsetable", "Uptight", "Useless", "Vague", "Vain", "Vampiric", "Vanilla", "Vast", "Verdant", "Vibrant", "Vicious", "Victorious", "Vile", "Violent", "Violet", "Viridian", "Vivacious", "Volatile", "Voluptuous", "Vulgar", "Wacky", "Wandering", "Wary", "Weak", "Weary", "Weepy", "Weird", "Wellamused", "Wellfrightened", "Wet", "White", "Wicked", "Wild", "Windy", "Wishful", "Withered", "Witless", "Witty", "Woke", "Wonderful", "Wondrous", "Worried", "Worrisome", "Wriggly", "Wrong", "YOLO", "Yellow", "Zany", "Zealous"
});

static std::array WORLD_NAME_LOCATIONS = std::to_array<std::string_view>({
    "Abode", "Abyss", "Accident", "Acres", "Afterworld", "Alcove", "Alley", "Apogee", "Arbor", "Archipelago", "Area", "Arena", "Armpit", "Artwork", "Asylum", "Backcountry", "Backwater", "Backwoods", "Badlands", "Bane", "Bank", "Barren", "Base", "Basin", "Bastion", "Bath Water", "Bay", "Bayou", "Beach", "Bed", "Bedrock", "Beyond", "Biosphere", "Blockchain", "Bluff", "Body", "Bog", "Boil", "Boingloings", "Boondocks", "Boonies", "Border", "Bottoms", "Boundary", "Bowel", "Bowels", "Bowl", "Box", "Breach", "Brewery", "Brook", "Bubble", "Bundle", "Bunker", "Burrow", "Bush", "Butte", "Camp", "Canal", "Canopy", "Canvas", "Canyon", "Cape", "Carton", "Center", "Chance", "Chaparral", "Chasm", "Chungus", "Citadel", "City", "Clearing", "Climax", "Cloudland", "Coast", "Colony", "Commune", "Confluence", "Constant", "Continent", "Convention", "Core", "Cosmos", "Couch", "Country", "County", "Court", "Courtyard", "Cove", "Crater", "Creek", "Crest", "Crick", "Croak", "Crossing", "Crossroads", "Crown", "Crypto", "Cult", "DLC", "Dab", "Dale", "Dam", "Daybreak", "Daydream", "Defecation", "Delight", "Dell", "Delta", "Den", "Depths", "Desert", "Dimension", "Dirt", "Discharge", "Disease", "District", "Dollop", "Domain", "Dome", "Door", "Dream", "Dreamland", "Dreamworld", "Drip", "Dump", "Dune", "Dungeon", "Easement", "Eater", "Eclipse", "Edge", "Egg", "Elevation", "Empire", "Empyrean", "Enclosure", "End", "Entity", "Essence", "Estate", "Estuary", "Eternity", "Ether", "Everglade", "Excrement", "Excretion", "Existence", "Expanse", "Exterior", "Eye", "Fable", "Fabrication", "Faith", "Fantasia", "Farm", "Field", "Figment", "Finger", "Firmament", "Fjord", "Flapper", "Flatland", "Flats", "Flex", "Flower", "Fluid", "Fold", "Folly", "Foot", "Foothold", "Forest", "Forge", "Fort", "Fortress", "Foundation", "Fountain", "Fraternity", "Front", "Frontier", "Galaxy", "Garden", "Gaze", "Geyser", "Glacier", "Glade", "Globe", "Grange", "Grassland", "Grave", "Graveyard", "Grounds", "Grove", "Gulf", "Gully", "Gutter", "Hamlet", "Harbor", "Harvest", "Haven", "Head", "Heap", "Heart", "Heaven", "Hedge", "Heights", "Hernia", "Hideout", "Highland", "Hill", "Hilltop", "Hinterland", "Hive", "Hole", "Hollow", "Homeland", "Honey", "Hoosegow", "Hope", "Horizon", "Hovel", "Hub", "Illusion", "Image", "Infection", "Interior", "Island", "Isle", "Islet", "Jalopy", "Jungle", "Keystone", "Kingdom", "Kiss", "Knoll", "Labor", "Labyrinth", "Lagoon", "Lair", "Lake", "Land", "Latte", "Leaf", "Legend", "Legs", "Lie", "Limbo", "Lining", "Locale", "Loch", "Magic", "Marsh", "Marshland", "Mass", "Maze", "Meadow", "Meridian", "Mesa", "Miasma", "Midland", "Mine", "Mirage", "Mire", "Mistake", "Mood", "Moon", "Moorland", "Morass", "Mortuary", "Moss", "Mound", "Mountain", "Mouth", "Myth", "NFT", "Nation", "Neighborhood", "Nest", "Niche", "Nightmare", "Nirvana", "Nooch", "Nugget", "Nursery", "Oasis", "Object", "Ocean", "Old Town Road", "Orchard", "Origin", "Outback", "Outland", "Outskirts", "Overworld", "Paintbrush", "Parable", "Paradise", "Park", "Passage", "Passenger", "Passing", "Pasture", "Patch", "Peak", "Pearl", "Pedestal", "Peninsula", "Picture", "Pie", "Pile", "Pinnacle", "Pit", "Place", "Plains", "Planet", "Plateau", "Plaza", "Plot", "Plumbus", "Point", "Polestar", "Pond", "Port", "Portrait", "Pothole", "Prairie", "Prison", "Province", "Pub", "Quagmire", "Quarantine", "Ranch", "Rapids", "Ravine", "Reach", "Reality", "Realm", "Reef", "Refuge", "Region", "Regret", "Remotes", "Residence", "Rest", "Retreat", "Ridge", "Rift", "Ring", "River", "Roost", "Root", "Route", "Run", "Sack", "Salt", "Salt Mine", "Sanctuary", "Savanna", "Schmeckle", "Scrubland", "Sea", "Seaside", "Secretion", "Section", "Sector", "Settlement", "Shallows", "Shanty", "Shantytown", "Sheet", "Shire", "Shoe", "Shore", "Shrine", "Shroud", "Shrubbery", "Shrublands", "Simp", "Site", "Sky", "Slice", "Slime", "Slope", "Slumber", "Snack", "Snap", "Sock", "Sod", "Soil", "Sorority", "Soup", "Source", "Space", "Span", "Speedrun", "Sphere", "Spiral", "Spring", "Square", "State", "Station", "Steppe", "Stick", "Sticks", "Stonk", "Story", "Strait", "Stream", "Stretch", "Study", "Suburb", "Summit", "Sunrise", "Sunset", "Swale", "Swamp", "Sweater", "Sweep", "Table", "Taiga", "Tale", "Tears", "Temple", "Terrace", "Terrain", "Terraria", "Terrarium", "Territory", "Thicket", "Throat", "Throne", "Tilt", "Timberland", "Tip", "Toilet", "Token", "Tomb", "Tongue", "Torch", "Touch", "Towel", "Town", "Tract", "Trail", "Tranch", "Trap", "Treasure", "Tree", "Trench", "Triangle", "Tributary", "Tropic", "Tundra", "Tunnel", "Turf", "Underbelly", "Undergrowth", "Underwear", "Underwood", "Universe", "Unknown", "Upland", "Utopia", "Vale", "Valley", "Vault", "Veldt", "Vibe", "Vicinity", "Vineyard", "Virus", "Vision", "Void", "Wall", "Ward", "Waste", "Wasteland", "Web", "Well", "Wetland", "Wharf", "Wilderness", "Wilds", "Wildwood", "Wonderland", "Wood", "Woodland", "Woods", "World", "Yard", "Yeet", "Zone",
});

static std::array WORLD_NAME_NOUNS = std::to_array<std::string_view>({
    "'Merica", "Ability", "Absurdity", "Accidents", "Acne", "Acorns", "Adamantite", "Adoration", "Adulthood", "Advantage", "Adventure", "Agony", "Alarm", "Alcohol", "Ale", "Allergies", "Amazement", "Angels", "Anger", "Angry Gamers", "Anguish", "Animal Carcasses", "Annoyance", "Anvils", "Anxiety", "Apples", "Apricots", "Argon", "Arrows", "Arsenic", "Arson", "Arthritis", "Asbestos", "Ash", "Assassins", "Assault", "Atrophy", "Awareness", "Awe", "Bacon", "Bad Advice", "Bad Decisions", "Bad Jokes", "Bad Luck", "Bad Omens", "Bad Times", "Bad Timing", "Balance", "Balloons", "Bamboo", "Bananas", "Bandits", "Bankruptcy", "Bark", "Bats", "Beauty", "Beenades", "Bees", "Beggars", "Beheadings", "Belief", "Betrayers", "Birds", "Birthdays", "Bitterness", "Bladders", "Blasphemy", "Blindness", "Blinkroot", "Blocks", "Blood", "Bloodletting", "Bloodshed", "Blossoms", "Bodies", "Bone", "Bone Spurs", "Bones", "Boom-Boom", "Boomers", "Boots", "Boredom", "Boulders", "Bragging Rights", "Brains", "Branches", "Bravery", "Bribery", "Bridges", "Brilliance", "Broken Bones", "Broken Dreams", "Broken Glass", "Broken Promises", "Bronies", "Bubbles", "Buckets", "Bugs", "Bums", "Bunnies", "Burglars", "Burnination", "Burning Hair", "Burnt Flesh", "Burnt Offerings", "Butchery", "Buttercups", "Butterflies", "Cacti", "Calmness", "Candy", "Care", "Carnage", "Carrion", "Casualty", "Cats", "Cattails", "Caves", "Celebration", "Cenx", "Chainsaws", "Change", "Chaos", "Charity", "Cheats", "Cherries", "Chests", "Childhood", "Children", "Chlorophyte", "Cilantro", "Clarity", "Clay", "Clentamination", "Cleverness", "Clouds", "Cobalt", "Coconuts", "Coffee", "Coins", "Coldness", "Comfort", "Compassion", "Compost", "Concern", "Confidence", "Confinement", "Confusion", "Contentment", "Convicts", "Copper", "Corpses", "Corruption", "Courage", "Creation", "Creatures", "Creepers", "Crests", "Crime", "Criminals", "Crimtane", "Crippling Depression", "Crooks", "Crows", "Crystals", "Cthulhu", "Curiosity", "Cursed Flames", "Cyborgs", "Dabbing", "Daisies", "Dank Memes", "Darkness", "Darts", "Daughters", "Dawn", "Daybloom", "Dead Bodies", "Deadbeats", "Death", "Deathweed", "Debauchery", "Debt", "Decapitation", "Decaying Meat", "Deceit", "Deception", "Dedication", "Defeat", "Defecation", "Degradation", "Delay", "Delusion", "Dementia", "Demilogic", "Demonite", "Demons", "Derangement", "Despair", "Desperation", "Destruction", "Diamond Hands", "Dijon Mustard", "Dillweeds", "Ding Dongs", "Dirt", "Dirt Blocks", "Disappointment", "Disgust", "Dishonesty", "Dismay", "Distortion", "Distribution", "Divorce", "Dogs", "Doom", "Dragonfruit", "Dragons", "Dread", "Dreams", "Drills", "Drums", "Ducks", "Dusk", "Dust", "Duty", "Dysphoria", "Ears", "Education", "Eggs", "Elderberries", "Elegance", "Envy", "Evasion", "Evil", "Exile", "Exploits", "Explosives", "Extortion", "Eyes", "FOMO", "Fable", "Face Monsters", "Facts", "Failures", "Fairies", "Faith", "Falling Stars", "False Idols", "False Imprisonment", "Falsehood", "Fame", "Famine", "Fantasy", "Fascination", "Fatality", "Fear", "Feathers", "Feces", "Felons", "Ferns", "Fiction", "Fiends", "Fingers", "Fireblossom", "Fireflies", "Fish", "Flails", "Flatulence", "Flatus", "Flesh", "Floof", "Flowers", "Flying Fish", "Flying Saucers", "Foam", "Food Poisoning", "Forgery", "Forgiveness", "Forks", "Fortitude", "Fraud", "Freaks", "Freckles", "Freedom", "Friendship", "Fright", "Frogs", "Frost", "Fruit", "Gangsters", "Garbage", "Garlic Bread", "Gears", "Gel", "Gemcorns", "Gen-Xers", "Gen-Yers", "Generation", "Ghosts", "Giggles", "Gingers", "Girls", "Glass", "Gloom", "Gluttony", "Goals", "Goblins", "Gold", "Goldfish", "Good Advice", "Good Times", "Gossip", "Grain", "Grandfathers", "Grandmothers", "Granite", "Grapefruit", "Grapes", "Grass", "Grasshoppers", "Graves", "Greed", "Grief", "Griefers", "Guitars", "Guts", "Hair", "Hamburgers", "Hammers", "Hands", "Happiness", "Happy Endings", "Hardship", "Harmony", "Harpies", "Hate", "Hatred", "Heart", "Heartache", "Hearts", "Heels", "Hellstone", "Herbs", "Heresy", "Hoiks", "Homicide", "Honey", "Hoodlums", "Hooks", "Hooligans", "Hope", "Hopelessness", "Hornets", "Horns", "Hornswoggle", "Horror", "Horrors", "Houses", "Humanity", "Humiliation", "Hurt", "Hysteria", "Ice", "Ichor", "Illness", "Imposter Syndrome", "Indictments", "Indigestion", "Indignity", "Infancy", "Infections", "Inflammation", "Inflation", "Injury", "Insanity", "Insects", "Intelligence", "Intestines", "Invasions", "Iron", "Irritation", "Isolation", "Item Duping", "Ivy", "Jaws", "Jealousy", "Jellyfish", "Joy", "Justice", "Karens", "Kidneys", "Kindness", "Kittens", "Knives", "Krypton", "Lamps", "Larceny", "Laughter", "Lava", "Lawsuits", "Lawyers", "Lead", "Learning", "Leaves", "Legend", "Legends", "Leggings", "Legs", "Lemons", "Leprosy", "Letdown", "Lethargy", "Liberty", "Lies", "Life", "Lightning Bugs", "Lilies", "Lilith", "Lilypads", "Lips", "Listening", "Litigation", "Livers", "Loathing", "Lombago", "Loneliness", "Loot", "Lore", "Losers", "Loss", "Love", "Luck", "Luggage", "Luminite", "Lungs", "Luxury", "Lyrics", "Mad Cow Disease", "Madness", "Maggots", "Man", "Mana", "Mangos", "Mania", "Mankind", "Manslaughter", "Marble", "Markets", "Marvel", "Mastication", "Maturity", "Medicine", "Melancholy", "Melodies", "Meme Lords", "Memes", "Mercy", "Meteorite", "Mice", "Microtransactions", "Midnight", "Midnight Ramen", "Milk", "Millennials", "Mimics", "Miracles", "Mirrors", "Misery", "Misfortune", "Missing Limbs", "Models", "Mom's Spaghetti", "Money", "Monotony", "Moonglow", "Moonlight", "Morons", "Mortality", "Moss", "Mourning", "Mouths", "Movement", "Muckers", "Mucus", "Mud", "Muggers", "Murder", "Murderers", "Mushrooms", "Music", "Mystery", "Myth", "Mythril", "Nausea", "Necromancers", "Necromancy", "Night", "Nightcrawlers", "Nightmares", "No Remorse", "Nocram", "Nostalgia", "Nudity", "Obscurity", "Obsidian", "Odor", "Ogres", "Oopsie Daisy", "Ooze", "Open Wounds", "Opportunity", "Options", "Oranges", "Orchids", "Organs", "Orichalcum", "Outlaws", "Over Confidence", "Owls", "Pad Thai", "Pain", "Palladium", "Panhandlers", "Panic", "Pansies", "Parasites", "Parties", "Party Time", "Partying", "Patience", "Peace", "Penguins", "Peril", "Perjury", "Perspiration", "Pickaxes", "Pickpockets", "Pineapples", "Pinky", "Piranha", "Piranhas", "Pirates", "Pixies", "Pizza", "Plantero", "Plants", "Platinum", "Pleasure", "Plums", "Politicians", "Ponies", "Potions", "Poverty", "Power", "Pride", "Prisms", "Privacy", "Promises", "Prophecy", "Psychology", "Public Speaking", "Puppies", "Purity", "Pus", "Rain", "Rainbows", "Ramen", "Rats", "Reality", "Redemption", "Regret", "Regurgitation", "Relaxation", "Relief", "Remorse", "Repugnance", "Rich Mahogany", "Riches", "Rocks", "Rope", "Roses", "Rotten Fruit", "Rotting Flesh", "Ruination", "Rumours", "Sacrifice", "Sacrilege", "Sadness", "Salesmen", "Sand", "Sandstone", "Sanity", "Sap", "Saplings", "Sashimi", "Sassages", "Satisfaction", "Sauce", "Scandal", "Scorpions", "Screams", "Seasons", "Seaweed", "Seclusion", "Secrecy", "Secrets", "Seeds", "Self-control", "Self-disgust", "Self-loathing", "Services", "Severed Heads", "Sewage", "Shade", "Shadows", "Shattered Hope", "Shenanigans", "Shivers", "Shiverthorn", "Shock", "Shrimp", "Silliness", "Silt", "Silver", "Sin", "Skeletons", "Skill", "Skin", "Skulls", "Sleep", "Slime", "Sloth", "Sloths", "Smiles", "Smoke", "Snails", "Snake Oil", "Snakes", "Snatchers", "Snow", "Solicitation", "Songs", "Sorrow", "Souls", "Sounds", "Spaghetti", "Sparkles", "Spears", "Speed", "Spicy Ramen", "Spikes", "Spirits", "Splinters", "Sponges", "Sprinkles", "Spurs", "Squid", "Squirrels", "Stagnant Water", "Starfruit", "Starvation", "Statues", "Stink Bugs", "Stone", "Stonks", "Strength", "Stress", "Strictness", "Stumps", "Suffering", "Sunflowers", "Sunshine", "Superstition", "Surprise", "Swagger", "Swindlers", "Swords", "Talent", "Tan Suits", "Taxation", "Taxes", "Teamwork", "Teddy's Bread", "Teeth", "Terror", "The Torch God", "Thieves", "Thinking", "Thorns", "Thunder", "Time", "Tin", "Tingling", "Tiredness", "Titanium", "Tombstones", "Torches", "Torment", "Torn Muscles", "Torture", "Traitors", "Tramps", "Tranquility", "Traps", "Trash", "Treasure", "Trees", "Trends", "Trouble", "Truffles", "Trunks", "Trust", "Trypophobia", "Tulips", "Tungsten", "Twigs", "Twilight", "Twisted Ankles", "Umbrellas", "Understanding", "Unjust Prices", "Upchuck", "Vagabonds", "Vampires", "Vanity", "Venom", "Victims", "Victory", "Villains", "Vines", "Violets", "Voilence", "Vomit", "Vultures", "Wands", "Wariness", "Warmth", "Wasps", "Waterleaf", "Weakness", "Wealth", "Webs", "Weeds", "Werewolves", "Whoopsies", "Wings", "Winners", "Winning", "Wires", "Wisdom", "Wizards", "Woe", "Wolves", "Wonder", "Wood", "Worlds", "Worms", "Worries", "Wrath", "Wrenches", "Wyverns", "Xenon", "Yoyos", "Zombies", "Zoomers", "the Ancients", "the Angler", "the Apple", "the Archer", "the Aunt", "the Axe", "the Baby", "the Ball", "the Balloon", "the Bat", "the Beast", "the Betrayed", "the Blender", "the Blood Moon", "the Bow", "the Bride", "the Brony", "the Bubble", "the Bunghole", "the Bunny", "the Cactus", "the Cloud", "the Coma", "the Corruptor", "the Crab", "the Dance", "the Dark", "the Dead", "the Demogorgon", "the Devourer", "the Drax", "the Ducks", "the Eclipse", "the Elderly", "the Fairy", "the Father", "the Fellow Kids", "the Flu", "the Fool", "the Foot", "the Frozen", "the Gift", "the Ginger", "the Goat", "the Goblin", "the Golem", "the Greatest Generation", "the Groom", "the Guest", "the Hammer", "the Hammush", "the Head", "the Heavens", "the Hipster", "the Hobo", "the Homeless", "the King", "the Law", "the Library", "the Lihzahrd", "the Lilith", "the Lizard King", "the Lost Generation", "the Manager", "the Merchant", "the Mirror", "the Monster", "the Moon", "the Mother", "the Mummy", "the Mushroom", "the Narc", "the Needy", "the Nude", "the Old One", "the Pandemic", "the Pickaxe", "the Picksaw", "the Pigron", "the Po Boy", "the Porcelain God", "the Princess", "the Prism", "the Prodigy", "the Prophecy", "the Pwnhammer", "the Queen", "the Ramen", "the Right", "the Scholar", "the Shark", "the Sickle", "the Sky", "the Snap", "the Snitch", "the Spelunker", "the Spirits", "the Staff", "the Stars", "the Stench", "the Stooge", "the Sun", "the Sword", "the Tooth", "the Tortoise", "the Tree", "the Trend", "the Undead", "the Unicorn", "the Union", "the Unknown", "the Varmint", "the Waraxe", "the Wise", "the World", "the Yoyo", "the Zoologist",
});

MainMenuState::MainMenuState() :
    m_camera(sge::CameraOrigin::Center, sge::CoordinateSystem {
        .up = sge::CoordinateDirectionY::Negative,
        .forward = sge::CoordinateDirectionZ::Negative,
    }),
    m_batch(sge::Engine::Renderer(), {
        .font_shader = Assets::GetShader(ShaderAsset::FontShader).ps,
        .enable_scissor = true
    })
{
    m_camera.set_viewport(App::GetWindowResolution());
    m_camera.set_zoom(1.0f);

    m_cursor.SetForegroundColor(sge::LinearRgba(1.0, 0.08, 0.58));
    m_cursor.SetBackgroundColor(sge::LinearRgba(0.9, 0.9, 0.9));

    m_background_renderer.init();

    m_batch.SetIsUi(true);

    setup_background();

    m_position_stack.push_back(MenuPosition::MainMenu);

    srand(std::time(nullptr));
}

MainMenuState::~MainMenuState() {
    m_batch.Destroy(sge::Engine::Renderer().Context());
    m_background_renderer.terminate();
}

void MainMenuState::setup_background() {
    m_background_layers.clear();

    m_background_layers.push_back(
        BackgroundLayer(BackgroundAsset::Background0, 2.0f)
            .set_anchor(sge::Anchor::TopLeft)
            .set_speed(0.0f, 0.08f)
            .set_fill_screen_height(true)
            .set_fill_screen_width(true)
            .set_is_ui(true)
    );

    const float pos = m_camera.viewport().y;

    m_background_layers.push_back(
        BackgroundLayer(BackgroundAsset::Background7, 1.5f)
            .set_anchor(sge::Anchor::BottomLeft)
            .set_y(pos - 150.0f)
            .set_speed(0.2f, 0.9f)
            .set_fill_screen_width(true)
            .set_is_ui(true)
    );
    m_background_layers.push_back(
        BackgroundLayer(BackgroundAsset::Background90, 1.5f)
            .set_anchor(sge::Anchor::BottomLeft)
            .set_speed(0.4f, 0.8f)
            .set_y(pos + 50.0f)
            .set_fill_screen_width(true)
            .set_is_ui(true)
    );
    m_background_layers.push_back(
        BackgroundLayer(BackgroundAsset::Background91, 1.5f)
            .set_anchor(sge::Anchor::BottomLeft)
            .set_speed(0.6f, 0.7f)
            .set_y(pos + 150.0f)
            .set_fill_screen_width(true)
            .set_is_ui(true)
    );
    m_background_layers.push_back(
        BackgroundLayer(BackgroundAsset::Background92, 1.5f)
            .set_anchor(sge::Anchor::BottomLeft)
            .set_speed(1.0f, 0.6f)
            .set_y(pos + 550.0f)
            .set_fill_screen_width(true)
            .set_is_ui(true)
    );
    // m_background_layers.push_back(
    //     BackgroundLayer(BackgroundAsset::Background112, 1.2f)
    //     .set_anchor(sge::Anchor::TopLeft)
    //         .set_speed(0.3f, 0.7f)
    //         .set_y(0.0f)
    //         .set_fill_screen_width(true)
    //         .set_is_ui(true)
    // );
}

void MainMenuState::Update() {
    UI::Update();

    m_cursor.Update(sge::Input::MouseScreenPosition());

    m_camera.set_position(m_camera.position() + glm::vec2(50.0f, 0.0f) * sge::Time::DeltaSeconds());

    for (BackgroundLayer& layer : m_background_layers) {
        if (layer.fill_screen_height()) {
            layer.set_height(m_camera.viewport().y);
        }
        if (layer.fill_screen_width()) {
            layer.set_width(m_camera.viewport().x);
        }
    }

    m_camera.update();

    if (sge::Input::JustPressed(sge::Key::Escape)) {
        set_previous_position();
    }

    update_logo();

    switch (position()) {
    case MenuPosition::CreateWorld:
        update_create_world();
    break;
    default: break;
    }
}

void MainMenuState::update_create_world() {
    m_name_input_data.update();
    m_seed_input_data.update();

    if (m_name_input_data.active() || m_seed_input_data.active()) {
        if (m_bar_timer.tick(sge::Time::Delta()).finished()) {
            m_text_input_bar_visible = !m_text_input_bar_visible;
        }
    }
}

void MainMenuState::update_logo() {
    m_logo_animation.tick(sge::Time::DeltaSeconds());

    m_logo_scale = LOGO_ANIM_MIN_SCALE + (LOGO_ANIM_MAX_SCALE - LOGO_ANIM_MIN_SCALE) * m_logo_animation.progress();
    m_logo_rotation = Quat::from_rotation_z(glm::radians(LOGO_ANIM_MIN_ROTATION + (LOGO_ANIM_MAX_ROTATION - LOGO_ANIM_MIN_ROTATION) * m_logo_animation.progress()));
}

void MainMenuState::Render() {
    m_batch.Reset();
    m_background_renderer.reset();

    draw_ui();

    for (const BackgroundLayer& layer : m_background_layers) {
        draw_background(layer);
    }

    sge::Renderer& renderer = sge::Engine::Renderer();

    renderer.Begin(m_camera);

    renderer.PrepareBatch(m_batch);
    renderer.UploadBatchData();

    renderer.BeginMainPass();
        renderer.Clear(LLGL::ClearValue(0.0f, 0.0f, 0.0f, 0.0f, 0.0f), LLGL::ClearFlags::ColorDepth);

        m_background_renderer.render();

        renderer.RenderBatch(m_batch);
    renderer.EndPass();

    renderer.End();
}

void MainMenuState::draw_main_menu() {
    using namespace widgets;

    const sge::Font& font = Assets::GetFont(FontAsset::AndyBold);

    UI::Container({
        .size = UiSize::Fill(),
        .self_alignment = Alignment::Center,
        .horizontal_alignment = Alignment::Center,
        .vertical_alignment = Alignment::Center,
    }, [&] {
        UI::Container({
            .gap = 24.0f,
            .orientation = LayoutOrientation::Vertical,
            .horizontal_alignment = Alignment::Center
        }, [&] {
            MenuButton(font, "Play", [this]() {
                set_next_position(MenuPosition::SelectWorld);
            });

            MenuButton(font, "Settings", [this]() {
                set_next_position(MenuPosition::Settings);
            });

            MenuButton(font, "Exit", [this]() {
                m_exit = true;
            });
        });
    });
}

void MainMenuState::draw_select_world() {
    using namespace widgets;

    const sge::Font& font = Assets::GetFont(FontAsset::AndyBold);

    UI::Container({
        .self_alignment = Alignment::Center,
        .horizontal_alignment = Alignment::Center
    }, [&] {
        UI::Container({
            .orientation = LayoutOrientation::Vertical,
            .horizontal_alignment = Alignment::Center
        }, [&] {
            UI::Element<UiTypeID::Panel>({
                .size = UiSize::Fixed(600.0f, 400.0f),
                .padding = UiRect::Axes(12.0f, 8.0f),
                .gap = 5.0f,
                .orientation = LayoutOrientation::Vertical,
                .horizontal_alignment = Alignment::Center,
                .scrollable = true,
            }, [&] {
                for (size_t i = 0; i < 15; ++i) {
                    WorldListItem(font);
                }
            });

            UI::Spacer(UiSize::Height(Sizing::Fixed(6.0f)));

            UI::Container({
                .size = UiSize::Width(Sizing::Fill()),
                .gap = 24.0f,
                .orientation = LayoutOrientation::Horizontal,
            }, [&] {
                Button(font, UiSize::Width(Sizing::Fill()), "Back", [this]() {
                    set_previous_position();
                });
                Button(font, UiSize::Width(Sizing::Fill()), "New", [this]() {
                    set_next_position(MenuPosition::CreateWorld);
                });
            });
        });

        UI::Element<UiTypeID::Panel>({
            .id = ID::Local("Header"),
            .padding = UiRect::Horizontal(12.0f),
            .offset = glm::vec2(0.0f, -35.0f)
        }, [&] {
            UI::SetCustomData(UiPanelData {
                .background_color = sge::LinearRgba(73, 94, 171),
                .border_color = sge::LinearRgba::black()
            });

            UI::Text<UiTypeID::Text>(font, sge::rich_text("Select World", 42.0f, sge::LinearRgba::white()));
        });
    });
}

void MainMenuState::set_random_world_name() {
    const int composition = rand() % 6;

    switch (composition) {
    case 0: {
        const std::string_view adj = get_random(WORLD_NAME_ADJECTIVES);
        const std::string_view loc = get_random(WORLD_NAME_LOCATIONS);
        const std::string_view noun = get_random(WORLD_NAME_NOUNS);
        m_name_input_data.set_text(temp_format("The {} {} of {}", adj, loc, noun));
    } break;
    case 1: {
        const std::string_view adj = get_random(WORLD_NAME_ADJECTIVES);
        const std::string_view loc = get_random(WORLD_NAME_LOCATIONS);
        const std::string_view noun = get_random(WORLD_NAME_NOUNS);
        m_name_input_data.set_text(temp_format("{} {} of {}", adj, loc, noun));
    } break;
    case 2: {
        const std::string_view adj = get_random(WORLD_NAME_ADJECTIVES);
        const std::string_view loc = get_random(WORLD_NAME_LOCATIONS);
        m_name_input_data.set_text(temp_format("The {} {}", adj, loc));
    } break;
    case 3: {
        const std::string_view adj = get_random(WORLD_NAME_ADJECTIVES);
        const std::string_view loc = get_random(WORLD_NAME_LOCATIONS);
        m_name_input_data.set_text(temp_format("{} {}", adj, loc));
    } break;
    case 4: {
        const std::string_view loc = get_random(WORLD_NAME_LOCATIONS);
        const std::string_view noun = get_random(WORLD_NAME_NOUNS);
        m_name_input_data.set_text(temp_format("The {} of {}", loc, noun));
    } break;
    case 5: {
        const std::string_view loc = get_random(WORLD_NAME_LOCATIONS);
        const std::string_view noun = get_random(WORLD_NAME_NOUNS);
        m_name_input_data.set_text(temp_format("{} of {}", loc, noun));
    } break;
    default:
        SGE_UNREACHABLE();
    }
}

void MainMenuState::set_random_seed() {
    static constexpr uint32_t LENGTH = 40;

    m_seed_input_data.clear();
    m_seed_input_data.text().reserve(LENGTH);

    for (uint32_t i = 0; i < LENGTH; ++i) {
        m_seed_input_data.text().push_back('0' + rand() % 10);
    }
}

void MainMenuState::draw_create_world() {
    using namespace widgets;

    const sge::Font& font = Assets::GetFont(FontAsset::AndyBold);

    UI::Container({
        .gap = 6.0f,
        .orientation = LayoutOrientation::Vertical,
        .self_alignment = Alignment::Center,
        .horizontal_alignment = Alignment::Center
    }, [&] {
        UI::Element<UiTypeID::Panel>({
            .id = ID::Local("Container"),
            .padding = UiRect::Axes(12.0f, 8.0f),
            .min_width = 600.0f,
            .gap = 6.0f,
            .orientation = LayoutOrientation::Vertical,
            .horizontal_alignment = Alignment::Center,
        }, [&] {
            UI::Container({
                .size = UiSize::Width(Sizing::Fill()),
                .gap = 6.0f,
                .orientation = LayoutOrientation::Horizontal
            }, [&] {
                UI::Container({
                    .size = UiSize::Width(Sizing::Fill()),
                    .gap = 6.0f,
                    .orientation = LayoutOrientation::Vertical
                }, [&] {
                    UI::Container({
                        .size = UiSize::Width(Sizing::Fill()),
                        .gap = 6.0f,
                        .orientation = LayoutOrientation::Horizontal,
                        .vertical_alignment = Alignment::Center
                    }, [&] {
                        IconButton(TextureAsset::UiIconRandomName, [this] {
                            set_random_world_name();
                        });

                        TextInput(m_name_input_data, m_text_input_bar_visible, font, UiSize::Fill());
                    });

                    UI::Container({
                        .size = UiSize::Width(Sizing::Fill()),
                        .gap = 6.0f,
                        .orientation = LayoutOrientation::Horizontal,
                        .vertical_alignment = Alignment::Center
                    }, [&] {
                        IconButton(TextureAsset::UiIconRandomSeed, [this] {
                            set_random_seed();
                        });

                        TextInput(m_seed_input_data, m_text_input_bar_visible, font, UiSize::Fill());
                    });
                });

                UI::AddElement<UiTypeID::WorldPreview>({
                    .size = UiSize::Fixed(94.0f, 94.0f),
                });
            });

            HorizontalSeparator(Sizing::Fill(), sge::LinearRgba::white().lerp(sge::LinearRgba(63, 65, 151), 0.85f) * 0.9f);
        });

        UI::Spacer(UiSize::Height(Sizing::Fixed(12.0f)));

        UI::Container({
            .size = UiSize::Width(Sizing::Fill()),
            .gap = 24.0f,
            .orientation = LayoutOrientation::Horizontal,
        }, [&] {
            Button(font, UiSize::Width(Sizing::Fill()), "Back", [this]() {
                set_previous_position();
            });
            Button(font, UiSize::Width(Sizing::Fill()), "Create", [this]() {
                m_world_selected = true;
            });
        });
    });
}

void MainMenuState::draw_ui() {
    UI::Start(RootDesc(m_camera.viewport()));

    UI::Container({
        .size = UiSize::Fill(),
    }, [&] {
        UI::AddElement<widgets::UiTypeID::Logo>({
            .size = UiSize::Fixed(Assets::GetTexture(TextureAsset::UiLogo).size()),
            .self_alignment = Alignment::TopCenter
        });

        switch (position()) {
        case MenuPosition::MainMenu:
            draw_main_menu();
        break;
        case MenuPosition::SelectWorld:
            draw_select_world();
        break;
        case MenuPosition::CreateWorld:
            draw_create_world();
        break;
        case MenuPosition::Settings:
        break;
        }
    });

    const std::vector<UiElement>& elements = UI::Finish();

    sge::Sprite sprite(Assets::GetTexture(TextureAsset::Stub));
    sge::NinePatch ninepatch(Assets::GetTexture(TextureAsset::UiPanelBackground), glm::uvec4(12));

    m_batch.BeginOrderMode();

    for (const UiElement& element : elements) {
        const sge::Order order = sge::Order(element.z_index);

        if (element.scissor_start) {
            m_batch.BeginScissorMode(element.scissor_data->area);
            continue;
        }

        if (element.scissor_end) {
            m_batch.EndScissorMode();
            continue;
        }

        switch (element.type_id) {
            case widgets::UiTypeID::Text: {
                const TextData* data = element.text_data;
                m_batch.DrawText(data->sections, data->sections_count, element.position, data->font, order);
            } break;

            case widgets::UiTypeID::Panel: {
                widgets::DrawPanel(element, m_batch, ninepatch);
            } break;

            case widgets::UiTypeID::CategoryPanel: {
                widgets::DrawCategoryPanel(element, m_batch, ninepatch);
            } break;

            case widgets::UiTypeID::Logo: {
                sprite.set_anchor(sge::Anchor::Center);
                sprite.set_texture(Assets::GetTexture(TextureAsset::UiLogo));
                sprite.set_position(element.position + element.size * 0.5f);
                sprite.set_custom_size(element.size);
                sprite.set_scale(m_logo_scale);
                sprite.set_rotation(m_logo_rotation);
                m_batch.DrawSprite(sprite, order);
            } break;

            case widgets::UiTypeID::Icon: {
                const widgets::UiIconData* data = static_cast<const widgets::UiIconData*>(element.custom_data);
                sprite.set_anchor(sge::Anchor::TopLeft);
                sprite.set_texture(Assets::GetTexture(data->icon));
                sprite.set_position(element.position);
                sprite.set_custom_size(element.size);
                sprite.set_rotation(glm::identity<glm::quat>());
                sprite.set_scale(1.0f);
                m_batch.DrawSprite(sprite, order);
            } break;

            case widgets::UiTypeID::Separator: {
                const widgets::UiSeparatorData* data = static_cast<const widgets::UiSeparatorData*>(element.custom_data);
                ninepatch.set_margin(glm::uvec4(2));
                ninepatch.set_texture(Assets::GetTexture(TextureAsset::UiSeparator1));
                ninepatch.set_anchor(sge::Anchor::TopLeft);
                ninepatch.set_position(element.position);
                ninepatch.set_size(element.size);
                ninepatch.set_color(data->color);
                m_batch.DrawNinePatch(ninepatch, order);
            } break;

            case widgets::UiTypeID::WorldPreview: {
                sprite.set_anchor(sge::Anchor::TopLeft);
                sprite.set_position(element.position + glm::vec2(4.0f));
                sprite.set_custom_size(element.size - glm::vec2(4.0f));
                sprite.set_color(sge::LinearRgba::white());
                sprite.set_rotation(glm::identity<glm::quat>());
                
                sprite.set_texture(Assets::GetTexture(TextureAsset::UiWorldPreviewDifficultyNormal1));
                m_batch.DrawSprite(sprite, sge::Order(order.value + 1));

                sprite.set_texture(Assets::GetTexture(TextureAsset::UiWorldPreviewEvilRandom));
                m_batch.DrawSprite(sprite, sge::Order(order.value + 2));

                sprite.set_texture(Assets::GetTexture(TextureAsset::UiWorldPreviewSizeLarge));
                m_batch.DrawSprite(sprite, sge::Order(order.value + 3));

                sprite.set_texture(Assets::GetTexture(TextureAsset::UiWorldPreviewDifficultyNormal2));
                m_batch.DrawSprite(sprite, sge::Order(order.value + 4));

                sprite.set_position(element.position);
                sprite.set_custom_size(element.size);
                sprite.set_texture(Assets::GetTexture(TextureAsset::UiWorldPreviewBorder));
                m_batch.DrawSprite(sprite, sge::Order(order.value + 5));
            } break;

            case widgets::UiTypeID::SliderHandle: {
                sprite.set_anchor(sge::Anchor::TopLeft);
                sprite.set_position(element.position);
                sprite.set_custom_size(element.size);
                sprite.set_color(sge::LinearRgba::white());
                sprite.set_rotation(glm::identity<glm::quat>());
                sprite.set_texture(Assets::GetTexture(TextureAsset::UiSliderHandle));
                m_batch.DrawSprite(sprite, order);
            } break;

            case widgets::UiTypeID::TextInput: {
                widgets::DrawTextInput(element, m_batch, sprite);
            } break;
        }
    }

    m_batch.EndOrderMode();

    m_batch.DrawSprite(m_cursor.Background());
    m_batch.DrawSprite(m_cursor.Foreground());
}

BaseState* MainMenuState::GetNextState() {
    if (m_exit)
        return nullptr;

    if (m_world_selected)
        return new InGameState();

    return this;
}

void MainMenuState::set_next_position(MenuPosition new_state) {
    m_position_stack.push_back(new_state);
}

void MainMenuState::set_previous_position() {
    if (m_position_stack.size() > 1) {
        m_position_stack.pop_back();
    }
}

void MainMenuState::draw_background(const BackgroundLayer& layer) {
    ZoneScoped;

    // TODO
    // const sge::Rect aabb = sge::Rect::from_top_left(layer.position() - layer.anchor().to_vec2() * layer.size(), layer.size());
    // if (!state.camera_frustums[layer.nonscale()].intersects(aabb)) return;

    if (layer.is_world()) {
        m_background_renderer.draw_world_layer(layer);
    } else {
        m_background_renderer.draw_layer(layer);
    }
}