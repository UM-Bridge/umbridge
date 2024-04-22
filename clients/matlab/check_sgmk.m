function check_sgmk()

% An aux function to check and download the sparse grids matlab kit

if exist('create_sparse_grid', 'file')==0
    % the sgmk is not found in the current path, so either we don't have it or we are not in the right subfolder

    if exist('sparse-grids-matlab-kit', 'dir')==0
        % there is no subfolder -> we don't have sgmk. Need to download it (if we haven't yet) and unzip

        if exist('sgmk.zip', 'file')==0
            % zip not found, let's download it
            
            try
                disp('Sparse Grids Matlab Kit not found, downloading from github (https://github.com/lorenzo-tamellini/sparse-grids-matlab-kit/)...');
                opts = weboptions; 
                opts.CertificateFilename=(''); % No idea why this is needed
                opts.Timeout = 100; % a larger timeout (in ms, default 5), just in case internet is slow today
                websave('sgmk.zip', 'https://github.com/lorenzo-tamellini/sparse-grids-matlab-kit/archive/refs/heads/main.zip', opts);
                disp('... done');
            catch ME
                fprintf('\nAutomatic download from github failed. Try manual download, from either github or project website https://sites.google.com/view/sparse-grids-kit. Download the latest version in .zip and name it sgmk.zip \n\n')                
                error(ME.identifier,ME.message);
            end
        end
        
        % now we have the zip, unzip
        try
            disp('unzipping ...');
            unzip('sgmk.zip','sparse-grids-matlab-kit');
            disp('done');
        catch ME
            error('%s. Automatic unzipping failed. Please extract sgmk.zip here', ME.message);
        end
    end
    
    % ready to add to path. Mind the double folder
    cd('sparse-grids-matlab-kit/sparse-grids-matlab-kit-main');
    addpath(genpath(pwd)) 
    disp('added Sparse Grids Matlab Kit to path')
    cd('../..');
    
else
    % the sgmk is found in the current path, we're good to go
    
    disp('Sparse Grids Matlab Kit is already in Matlab''s path')
end

