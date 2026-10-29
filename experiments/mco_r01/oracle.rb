#!/usr/bin/env ruby
# Independent reference: Ruby JSON parser, exact decimal values, schema subset
# interpreter and direct Boolean permission. No candidate/review helpers imported.
require 'json'
require 'bigdecimal'
require 'base64'
require 'digest'

class WireFailure < StandardError; end
class UniqueObject < Hash
  def []=(key, value)
    raise WireFailure, 'duplicate_key' if key?(key)
    super
  end
end

def identity(kind, raw)
  'sha256:' + Digest::SHA256.hexdigest(kind + "\0" + raw)
end

def depth(value)
  children = value.is_a?(Hash) ? value.values : value.is_a?(Array) ? value : nil
  children ? 1 + (children.map { |x| depth(x) }.max || 0) : 0
end

def numeric?(value)
  value.is_a?(Integer) || value.is_a?(BigDecimal)
end

def integer?(value)
  value.is_a?(Integer) || (value.is_a?(BigDecimal) && value.finite? && value.frac.zero?)
end

# Only the keywords present in the pinned, meta-validated schemas are supported.
# An unsupported future schema is a blocked experiment, not a silently valid one.
def schema_errors(value, schema, root = schema, path = '')
  supported = %w[$schema $defs $ref type additionalProperties required properties const pattern minLength maxLength enum minItems maxItems uniqueItems items minimum maximum]
  raise "unsupported schema keywords: #{schema.keys - supported}" unless (schema.keys - supported).empty?
  return schema_errors(value, root.fetch('$defs').fetch('ref'), root, path) if schema['$ref'] == '#/$defs/ref'
  raise 'nonlocal schema reference' if schema['$ref']
  errors = []
  add = ->(keyword) { errors << {'keyword'=>keyword, 'path'=>path} }
  type = schema['type']
  correct = case type
  when nil then true
  when 'object' then value.is_a?(Hash)
  when 'array' then value.is_a?(Array)
  when 'string' then value.is_a?(String)
  when 'integer' then integer?(value)
  else raise "unsupported type #{type}"
  end
  unless correct
    add.call('type')
    return errors
  end
  add.call('const') if schema.key?('const') && value != schema['const']
  add.call('enum') if schema['enum'] && !schema['enum'].include?(value)
  if value.is_a?(String)
    add.call('pattern') if schema['pattern'] && !Regexp.new(schema['pattern']).match?(value)
    add.call('minLength') if schema['minLength'] && value.length < schema['minLength']
    add.call('maxLength') if schema['maxLength'] && value.length > schema['maxLength']
  end
  if numeric?(value)
    add.call('minimum') if schema['minimum'] && value < schema['minimum']
    add.call('maximum') if schema['maximum'] && value > schema['maximum']
  end
  if value.is_a?(Array)
    add.call('minItems') if schema['minItems'] && value.length < schema['minItems']
    add.call('maxItems') if schema['maxItems'] && value.length > schema['maxItems']
    add.call('uniqueItems') if schema['uniqueItems'] && value.uniq.length != value.length
    value.each_with_index { |v,i| errors.concat(schema_errors(v,schema['items'],root,path+'/'+i.to_s)) } if schema['items']
  end
  if value.is_a?(Hash)
    (schema['required'] || []).each { |key| add.call('required') unless value.key?(key) }
    props = schema['properties'] || {}
    add.call('additionalProperties') if schema['additionalProperties'] == false && !(value.keys - props.keys).empty?
    props.each { |key,sub| errors.concat(schema_errors(value[key],sub,root,path+'/'+key)) if value.key?(key) }
  end
  errors
end

def inspect_wire(raw, limit, schema)
  return [nil, {'profile'=>'not_observed','schema'=>'not_run','schema_errors'=>[]}] if raw.nil?
  begin
    raise WireFailure, 'byte_limit' if raw.bytesize > limit
    text = raw.dup.force_encoding(Encoding::UTF_8)
    raise WireFailure, 'utf8' unless text.valid_encoding?
    raise WireFailure, 'bom' if text.start_with?("\uFEFF")
    begin
      value = JSON.parse(text, decimal_class: BigDecimal, object_class: UniqueObject,
                         allow_nan: false, max_nesting: false)
    rescue JSON::ParserError => error
      # Ruby rejects an unpaired escaped surrogate while Python accepts it as
      # a string. Preserve the contract's scalar-profile diagnosis, not a generic
      # syntax label, for this independently identified parser condition.
      raise WireFailure, error.message.include?('surrogate') ? 'unicode_scalar' : 'json_syntax'
    end
    valid_scalar = lambda do |v|
      case v
      when String then v.valid_encoding? && v.codepoints.none? { |cp| (0xd800..0xdfff).include?(cp) }
      when Array then v.all? { |x| valid_scalar.call(x) }
      when Hash then v.all? { |k,x| valid_scalar.call(k) && valid_scalar.call(x) }
      else true
      end
    end
    raise WireFailure, 'unicode_scalar' unless valid_scalar.call(value)
    raise WireFailure, 'depth_limit' if depth(value) > 4
    errors = schema_errors(value,schema)
    [value, {'profile'=>'ok','schema'=>errors.empty? ? 'valid' : 'invalid','schema_errors'=>errors}]
  rescue WireFailure => error
    [nil, {'profile'=>error.message,'schema'=>'not_run','schema_errors'=>[]}]
  end
end

class Reference
  HARD = %w[MODEL_REJECT EVIDENCE_REFUTED EVIDENCE_CONFLICT EVIDENCE_INVALID CAPABILITY POLICY_REVOKED REPLAY STALE_POLICY BUDGET DEADLINE].freeze
  def initialize(root)
    @root = root
    @objects = JSON.parse(File.read(File.join(root,'corpus','objects.json')))
    @schemas = %w[request response].map { |k| JSON.parse(File.read(File.join(root,'schemas',k+'.json'))) }
  end

  def evaluate(c, context)
    raw = Base64.strict_decode64(c.fetch('request_bytes_base64'))
    output = c['response_bytes_base64'].nil? ? nil : Base64.strict_decode64(c['response_bytes_base64'])
    req,rw = inspect_wire(raw,8192,@schemas[0])
    res,ow = inspect_wire(output,4096,@schemas[1])
    valid_output = ow['profile']=='ok' && ow['schema']=='valid'
    stored = valid_output ? res['recommendation'] : nil
    result = {'wire_review'=>{'request'=>rw,'response'=>ow},'stored_interpretation'=>stored,
              'interpretation'=>nil,'guards'=>nil,'diagnostics'=>[],'evidence_observations'=>[]}
    stop = ->(stage, reason) {
      result.merge('pipeline_stage'=>stage,'decision'=>'D','reasons'=>[reason],
                   'diagnostics'=>[{'code'=>reason,'subject'=>identity('request',raw)}])
    }
    return stop.call('RequestProfile','INVALID_REQUEST') unless rw['profile']=='ok'
    return stop.call('RequestSchema','INVALID_REQUEST') unless rw['schema']=='valid'
    binding = identity('request',raw)==context['expected_request_ref'] &&
      req['model_ref']==context['approved_model_ref'] && req['artifact_ref']==context['expected_artifact_ref'] &&
      req['policy_ref']==context['expected_policy_ref'] && req['deadline_tick']==context['deadline_tick'] &&
      req['evidence_refs']==req['evidence_refs'].sort &&
      req['evidence_refs']==context['required_evidence'].map { |x| x['ref'] }
    return stop.call('RequestBinding','BINDING') unless binding
    if c['response_event']!='Response'
      raise 'inconsistent failure event bytes' unless output.nil? && c['output_ref'].nil?
      return stop.call('ModelFailure','MODEL_FAILURE')
    end
    raise 'Response event missing bytes' if output.nil?
    return stop.call('ResponseProfile','INVALID_RESPONSE') unless ow['profile']=='ok'
    return stop.call('ResponseSchema','INVALID_RESPONSE') unless ow['schema']=='valid'
    result['interpretation']=stored
    echoes = res['request_id']==req['request_id'] && res['request_ref']==identity('request',raw) &&
      res['model_ref']==req['model_ref'] && res['evidence_refs']==req['evidence_refs'] &&
      context['observed_model_ref']==context['approved_model_ref'] &&
      context['observed_provider']==context['approved_provider']
    return stop.call('ResponseBinding','BINDING') unless echoes
    facts = [[],[],[],[],[]]
    subjects = [[],[],[],[],[]]
    add = ->(i,code,subject=identity('request',raw)) { facts[i] << code; subjects[i] << subject }
    add.call(0,'MODEL_REJECT') if stored=='reject'
    add.call(0,'MODEL_REVIEW') if stored=='review'
    evidence_ok = true
    context['required_evidence'].each do |need|
      key = context['registry'][need['ref']]
      code = nil
      if key.nil?
        code='EVIDENCE_MISSING'
        result['evidence_observations'] << {'ref'=>need['ref'],'validity'=>'missing','pair'=>nil}
      else
        object = @objects.fetch(key)
        bytes = Base64.strict_decode64(object['bytes_base64'])
        obs = JSON.parse(bytes)
        intact = identity('evidence',bytes)==need['ref'] &&
          %w[artifact_ref predicate source epoch].all? { |field| obs[field]==need[field] } &&
          [true,false].include?(obs['supports']) && [true,false].include?(obs['refutes'])
        result['evidence_observations'] << {'ref'=>need['ref'],'validity'=>intact,'pair'=>[obs['supports'],obs['refutes']],'observed_ref'=>identity('evidence',bytes)}
        code = if !intact then 'EVIDENCE_INVALID'
        elsif obs['supports'] && obs['refutes'] then 'EVIDENCE_CONFLICT'
        elsif obs['refutes'] then 'EVIDENCE_REFUTED'
        elsif !obs['supports'] then 'EVIDENCE_MISSING'
        end
      end
      evidence_ok &&= code.nil?
      add.call(1,code,need['ref']) if code
    end
    grant = {'caller'=>context['caller'],'model_ref'=>req['model_ref'],'operation'=>'append_report',
             'artifact_ref'=>req['artifact_ref'],'policy_ref'=>req['policy_ref']}
    grant_ok = context['policy_present'] && context['grant']==grant
    add.call(2,'CAPABILITY') unless grant_ok
    add.call(2,'POLICY_REVOKED') if context['policy_present'] && context['policy_state']=='revoked'
    add.call(2,'POLICY_UNAVAILABLE') if context['policy_present'] && context['policy_state']=='unavailable'
    add.call(3,'REPLAY') if context['replay_state']=='spent'
    add.call(3,'STALE_POLICY') if req['policy_ref']!=context['current_policy_ref']
    add.call(3,'REPLAY_UNAVAILABLE') if context['replay_state']=='unavailable'
    allowance = [req['budget_units'],context['remaining_units']].min
    add.call(4,'BUDGET') if allowance < 1
    add.call(4,'DEADLINE') if context['now_tick'] > req['deadline_tick']
    permit = stored=='endorse' && evidence_ok && grant_ok && context['policy_state']=='active' &&
      context['replay_state']=='fresh' && req['policy_ref']==context['current_policy_ref'] &&
      allowance>=1 && context['now_tick']<=req['deadline_tick']
    codes = facts.flatten
    decision = permit ? 'P' : codes.any? { |x| HARD.include?(x) } ? 'D' : 'U'
    raise 'direct predicate inconsistent with fact classification' unless (decision=='P')==codes.empty?
    guards = ['P'] + facts.map { |fs| fs.empty? ? 'P' : fs.any? { |x| HARD.include?(x) } ? 'D' : 'U' }
    diag = facts.each_with_index.flat_map { |fs,i| fs.each_with_index.map { |code,j| {'code'=>code,'subject'=>subjects[i][j]} } }
    result.merge('pipeline_stage'=>'Guards','guards'=>guards,'decision'=>decision,'reasons'=>codes,'diagnostics'=>diag)
  end

  def run(c)
    context = Marshal.load(Marshal.dump(c.fetch('context')))
    initial = evaluate(c,context)
    state = {'policy_state'=>context['policy_state'],'replay_state'=>context['replay_state'],
             'remaining_units'=>context['remaining_units'],'rows'=>context['initial_rows'].length,
             'owner_sequence'=>context['initial_rows'].length}
    journal = [{'sequence'=>0,'event'=>'Decision','decision'=>initial['decision'],'reasons'=>initial['reasons'],
                'before'=>state.dup,'after'=>state.dup,'effect_dispatches'=>0,'new_rows'=>0}]
    admissions = []
    c.fetch('events').each do |event|
      before=state.dup
      result=nil;dispatches=0;added=0
      if event=='V'
        state['policy_state']='revoked'
      else
        context.merge!(state.reject { |k,v| %w[rows owner_sequence].include?(k) })
        result=evaluate(c,context)
        if %w[A B].include?(event)
          dispatches=1 if result['decision']=='P'
          if dispatches==1 && !context['sink_fault']
            added=1;state['rows']+=1;state['owner_sequence']+=1
            state['remaining_units']-=1;state['replay_state']='spent'
          end
          admissions << {'event'=>event,'decision'=>result['decision'],'reasons'=>result['reasons'],
                         'effect_dispatches'=>dispatches,'new_rows'=>added}
        end
      end
      journal << {'sequence'=>journal.length,'event'=>event,'decision'=>result && result['decision'],
                  'reasons'=>result ? result['reasons'] : [],'before'=>before,'after'=>state.dup,
                  'effect_dispatches'=>dispatches,'new_rows'=>added}
    end
    primary=admissions.find { |a| a['event']=='A' }
    final_decision=primary ? primary['decision'] : initial['decision']
    final_reasons=primary ? primary['reasons'] : initial['reasons']
    added=admissions.map { |a| a['new_rows'] }.inject(0,:+)
    outcome=if primary && primary['new_rows']==1 then 'Committed'
    elsif primary && primary['effect_dispatches']==1 then 'EffectFailed'
    elsif final_decision=='U' then 'Deferred' else 'Rejected' end
    initial.merge('final_decision'=>final_decision,'final_reasons'=>final_reasons,'admissions'=>admissions,
                  'effect_dispatches'=>admissions.map { |a| a['effect_dispatches'] }.inject(0,:+),'new_rows'=>added,
                  'final_state'=>state.reject { |k,v| k=='owner_sequence' },'outcome'=>outcome,
                  'outcome_reasons'=>outcome=='EffectFailed' ? ['SINK_FAILURE'] : %w[Rejected Deferred].include?(outcome) ? final_reasons : [],
                  'failure_subtag'=>c['response_event']=='Response' ? nil : c['response_event'],
                  'transition_expectations'=>journal)
  end
end

def canonical(value)
  sort = lambda do |v|
    if v.is_a?(Hash)
      v.keys.sort.each_with_object({}) { |key,out| out[key]=sort.call(v[key]) }
    elsif v.is_a?(Array)
      v.map { |x| sort.call(x) }
    else v
    end
  end
  JSON.generate(sort.call(value), ascii_only: true)
end

class Reference
  def plan(base, order)
    valid=order.chars.sort==%w[A B R V] && order.index('R')<order.index('A') && order.index('A')<order.index('B')
    return {'order'=>order,'reachable'=>false,'classification'=>'INVALID_ORDER','executed_events'=>[],'new_rows'=>nil} unless valid
    c=base.merge('events'=>order.chars)
    result=run(c)
    {'order'=>order,'reachable'=>true,'classification'=>'EXPECTED_TRANSITIONS',
     'executed_events'=>result['transition_expectations'].drop(1).each_with_index.map { |row,i| row.merge('sequence'=>i) },'new_rows'=>result['new_rows'],'final_state'=>result['final_state']}
  end

  def reconstruct(observed, c)
    # Never read c['expected'] or trust observed['summary']['decision'] here.
    expected=run(c)
    differences=expected.keys.select { |key| observed.fetch('summary')[key]!=expected[key] }
    differences << 'input_context' unless observed['input_context']==c['context']
    differences << 'request_ref' unless observed['request_ref']==identity('request',Base64.strict_decode64(c['request_bytes_base64']))
    output=c['response_bytes_base64'].nil? ? nil : identity('output',Base64.strict_decode64(c['response_bytes_base64']))
    differences << 'output_ref' unless observed['output_ref']==output
    differences << 'journal' unless observed['journal']==expected['transition_expectations']
    arm=observed.fetch('arm')
    raise 'unapproved arm name' unless %w[A B].include?(arm)
    source=File.expand_path('../../../experiments/mco_r01',@root)
    fingerprints=['wire.py','arm_'+arm.downcase+'.py','driver.py'].each_with_object({}) do |file,h|
      h[file]=Digest::SHA256.hexdigest(File.binread(File.join(source,file)))
    end
    verifier=identity('artifact',canonical(fingerprints))
    differences << 'verifier_ref' unless observed['verifier_ref']==verifier
    rows=Marshal.load(Marshal.dump(c['context']['initial_rows']))
    receipts=[]
    request=JSON.parse(Base64.strict_decode64(c['request_bytes_base64']))
    expected['transition_expectations'].each_with_index do |event,index|
      next unless event['new_rows']==1
      prefix={'case_id'=>c['case_id'],'fixture_sha256'=>c['fixture_sha256'],'arm'=>arm,
              'verifier_ref'=>verifier,'journal'=>expected['transition_expectations'][0..index]}
      row={'request_ref'=>identity('request',Base64.strict_decode64(c['request_bytes_base64'])),
           'artifact_ref'=>request['artifact_ref'],'model_ref'=>request['model_ref'],'policy_ref'=>request['policy_ref'],
           'output_ref'=>identity('output',Base64.strict_decode64(c['response_bytes_base64'])),
           'verifier_ref'=>verifier,'execution_ref'=>identity('execution',canonical(prefix)),
           'decision'=>'P','outcome'=>'Committed','owner_sequence'=>event['after']['owner_sequence']}
      rows << row
      receipts << {'row'=>row,'receipt_ref'=>identity('receipt',canonical(row))}
    end
    differences << 'ledger_rows' unless rows==observed['ledger_rows']
    differences << 'receipts' unless receipts==observed['receipts']
    {'case_id'=>c['case_id'],'valid'=>differences.empty?,'mismatches'=>differences.uniq,
     'reconstructed_rows'=>rows.length,'observed_rows'=>observed['ledger_rows'].length}
  end
end

if __FILE__==$0
  root=File.expand_path(ARGV.fetch(0))
  reference=Reference.new(root)
  cases=File.readlines(File.join(root,'corpus','cases.jsonl')).map { |line| JSON.parse(line) }
  if ARGV[1]=='--reconstruct'
    observation=JSON.parse(File.read(ARGV.fetch(2)))
    catalog=cases.each_with_object({}) { |c,h| h[c['case_id']]=c }
    inventory=[]
    inventory << 'case inventory' unless observation['cases'].map { |o| o['case_id'] }==cases.map { |c| c['case_id'] }
    inventory << 'plan inventory' unless observation['plans'].map { |o| o['order'] }.sort==%w[R V A B].permutation.map(&:join).sort
    inventory << 'guard inventory' unless observation['guards'].map { |o| o['guards'] }.sort==%w[D U P].repeated_permutation(6).map(&:join).sort
    inventory << 'pair inventory' unless observation['pairs'].map { |o| o['values'] }.sort==%w[D U P].repeated_permutation(2).map(&:join).sort
    inventory << 'triple inventory' unless observation['triples'].map { |o| o['values'] }.sort==%w[D U P].repeated_permutation(3).map(&:join).sort
    inventory << 'arm binding' unless %w[A B].include?(observation['arm']) && observation['cases'].all? { |o| o['arm']==observation['arm'] }
    choices=observation.fetch('proposal_choices',[])
    if ARGV[3]
      trace=JSON.parse(File.read(ARGV[3]))
      inventory << 'trusted proposal trace' unless choices.map { |o| o['case_id'] }==trace['choices']
    elsif !choices.empty?
      inventory << 'missing independent proposal trace'
    end
    unless inventory.empty?
      puts JSON.pretty_generate({'valid'=>false,'inventory_errors'=>inventory,'reported_decisions_trusted'=>false})
      exit 2
    end
    checks=(observation['cases']+observation.fetch('proposal_choices',[])).map { |o| reference.reconstruct(o,catalog.fetch(o['case_id'])) }
    plans=observation['plans'].map do |o|
      expected=reference.plan(cases.first,o['order'])
      mismatches=expected.keys.select { |key| expected[key]!=o[key] }
      reconstructed=o['reachable'] ? reference.reconstruct(o['observation'],cases.first.merge('events'=>o['order'].chars)) : nil
      {'order'=>o['order'],'valid'=>mismatches.empty? && (reconstructed.nil? || reconstructed['valid']),
       'mismatches'=>mismatches,'reconstruction'=>reconstructed}
    end
    truth=->(xs) { xs.include?('D') ? 'D' : xs.all? { |x| x=='P' } ? 'P' : 'U' }
    algebra=[]
    observation['guards'].each { |row| algebra << ['guard',row['guards']] unless truth.call(row['guards'].chars)==row['summary'] }
    observation['pairs'].each { |row| algebra << ['pair',row['values']] unless truth.call(row['values'].chars)==row['summary'] }
    observation['triples'].each { |row| algebra << ['triple',row['values']] unless row['left']==truth.call(row['values'].chars) && row['right']==truth.call(row['values'].chars) }
    ok=checks.all? { |x| x['valid'] } && plans.all? { |x| x['valid'] } && algebra.empty?
    puts JSON.pretty_generate({'valid'=>ok,'reported_decisions_trusted'=>false,'checks'=>checks,'plans'=>plans,'algebra_mismatches'=>algebra})
    exit(ok ? 0 : 2)
  end
  results=cases.map do |c|
    actual=reference.run(c)
    differences=c['expected'].keys.select { |key| actual[key]!=c['expected'][key] }
    {'case_id'=>c['case_id'],'actual'=>actual,'mismatched_fields'=>differences}
  end
  plans=File.readlines(File.join(root,'corpus','event-permutations.jsonl')).map do |line|
    expected=JSON.parse(line);actual=reference.plan(cases.first,expected['order'])
    {'order'=>expected['order'],'actual'=>actual,'mismatched_fields'=>expected.keys.select { |key| key!='reason' && actual[key]!=expected[key] }}
  end
  ok=results.all? { |x| x['mismatched_fields'].empty? } && plans.all? { |x| x['mismatched_fields'].empty? }
  puts JSON.pretty_generate({'oracle'=>'Ruby direct-predicate reference','ruby'=>RUBY_VERSION,'json'=>JSON::VERSION,
                            'cases'=>results.length,'agreement'=>results.count { |x| x['mismatched_fields'].empty? },
                            'disagreements'=>results.count { |x| !x['mismatched_fields'].empty? },'results'=>results,'plans'=>plans})
  exit(ok ? 0 : 2)
end
