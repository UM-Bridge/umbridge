function status_ok = check_error(json)

status_ok = true;
if (isfield(json, 'error'))
    error('UMBRIDGE error %s: %s', json.error.type, json.error.message);
end

end
