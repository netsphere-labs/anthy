$KCODE = "EUC"

class SegmentClass
  def initialize(name, index, parents = [])
    @name = name
    @index = index
    @parents = parents
    @children = []
    @parents.each {|parent| parent.children << self}
    @transition_probability = {}
    @transition_probability.default = 0
  end
  attr_reader :name, :index, :parents, :children, :transition_probability

end

class SentenceData
  Segment = Struct.new(:name, :class_name)
  def initialize(segments = [])
    @segments = segments
  end
  attr_accessor :segments
end

class Corpus
  def initialize
    @segment_classes = {}  
  end
  attr_reader :segment_classes

  SENTENCE_HEAD = "文頭"
  SENTENCE_TAIL = "文末"

  def load_segment_class_from_file(filename)
    @segment_classes[SENTENCE_HEAD] ||= SegmentClass.new(SENTENCE_HEAD, 0)
    @segment_classes[SENTENCE_TAIL] ||= SegmentClass.new(SENTENCE_TAIL, 1)

    index = 2    
    File.open(filename) do |file|

      current_parent = nil
      previous_class = nil
      current_class = nil
      parent_stack = [current_parent]
      previous_space = 0
      while line = file.gets
        line.chomp!
        line = $' if line =~ /#.*/
        next if line =~ /^([\s\t]*)$/
        line =~ /^([\s\t]*)(.*)[\s\t]*$/
        current_space = $1.length
        current_name = $2
        unless @segment_classes[current_name]
          @segment_classes[current_name] = SegmentClass.new(current_name, index)
          $stderr << "#{current_name}, #{index}\n"
          index += 1
        end
        current_class = @segment_classes[current_name]

        if previous_space == current_space
        elsif previous_space < current_space
          current_parent = previous_class
          parent_stack << current_parent
        else
          current_parent = parent_stack[current_space]
          parent_stack = parent_stack[0..current_space]
        end
        if current_parent
          current_class.parents << current_parent
        end

#        expand_class(current_parent, current_class)
        previous_space = current_space
        previous_class = current_class
      end
    end
    return index
  end


=begin
  def expand_class(parent_class, child_class)
    return unless parent_class
    return if parent_class.name == ''

    new_classes = []

    @segment_classes.each do |key, segment_class|
      key_elements = key.split('+')
      next if key_elements.length == 1

      index = key_elements.index(parent_class.name)
      if index && key != parent_class.name
        before = key_elements[0...index].join('+')
        after = key_elements[(index+1)..-1].join('+')
        
        new_class_name = ''
        unless before == ''
          new_class_name += (before + '+')
        end
        new_class_name += child_class.name
        unless after == ''
          new_class_name += ('+' + after)
        end
        
        new_class = SegmentClass.new(new_class_name)
        new_class.parents << segment_class
        new_classes << new_class
      end
    end
    new_classes.each do |new_class|
      @segment_classes[new_class.name] = new_class
    end
  end
=end
  def store_sentence_from_file(filename)
    File.open(filename) do |file|
      while line = file.gets
        line.chomp!
        line = $' if line =~ /#.*/
        next if line =~ /^([\s\t]*)$/
        
        str_segments = line.split 
        segments = []
        for i in 0...(line.split.length / 2)
          segments << SentenceData::Segment.new(str_segments[i * 2],
                                                str_segments[i * 2 + 1])
        end
        sentence_data = SentenceData.new(segments)
        store_sentence(sentence_data)
      end
    end
  end

  def store_sentence(sentence_data)
    sentence_head = @segment_classes[SENTENCE_HEAD]
    sentence_tail = @segment_classes[SENTENCE_TAIL]

    last_segment_class = @segment_classes[sentence_data.segments[0].class_name]
    sentence_head.transition_probability[last_segment_class] += 1

    sentence_data.segments[1..-1].each do |segment|
      current_segment_class = @segment_classes[segment.class_name]
      unless current_segment_class
        $stderr << "no class for #{segment.class_name}"
      end
      last_segment_class.transition_probability[current_segment_class] += 1
      last_segment_class.parents.each do |parent|
        parent.transition_probability[current_segment_class] += 1
      end
      last_segment_class = current_segment_class
    end
    last_segment_class.transition_probability[sentence_tail] += 1
  end
end

corpus = Corpus.new
size = corpus.load_segment_class_from_file(ARGV[0])
corpus.store_sentence_from_file(ARGV[1])

matrix = Array.new(size)
size.times do |i|
  matrix[i] = Array.new(size, 0)
end

corpus.segment_classes.each do |key, segment_class|
  sum = 0
  segment_class.transition_probability.each do |segment, count|
    if segment.children.size == 0
      sum += count
    end
  end
  segment_class.transition_probability.each do |segment, count|
    matrix[segment_class.index][segment.index] = count.to_f / sum
  end
end

matrix.each do |row|
  zero_num = 0
  row.each do |element|
    if element == 0
      zero_num += 1
    end
  end
  row.map! do |element|
    if element == 0
      0.0001 / zero_num
    else
      0.9999 * element
    end
  end
end

matrix.each do |row|
  print "{"
  print row.join(',')
  print "},\n"
end
